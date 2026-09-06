#include "ui/AppController.hpp"

#include "platform/CaptureOverlay.hpp"
#include "platform/Cursor.hpp"
#include "platform/ForegroundProbe.hpp"
#include "platform/HighResolutionTimer.hpp"
#include "platform/HotkeyService.hpp"
#include "platform/PointPicker.hpp"
#include "platform/SendInputSink.hpp"
#include "platform/ThreadTuning.hpp"

#include <atomic>
#include <mutex>

namespace deuca::ui
{
namespace
{

/// Cadence de départ : dix clics par seconde.
///
/// Modeste à dessein. Une application qui s'ouvre réglée sur plusieurs
/// milliers de clics par seconde transforme la première pression du raccourci
/// en accident.
constexpr int kDefaultIntervalMilliseconds = 100;

} // namespace

struct AppController::Impl
{
    platform::HighResolutionTimer timer;
    platform::SendInputSink sink;
    platform::ForegroundProbe probe;
    RateGovernor governor;
    ClickEngine engine;

    /// L'asservissement est actif par défaut.
    ///
    /// C'est le comportement qui protège la cible, et le désactiver relève du
    /// choix éclairé : une case à cocher, pas un réglage par défaut.
    std::atomic<bool> governorEnabled{true};

    // Protège les démarrages et arrêts. Le raccourci global agit depuis son
    // propre fil : sans ce verrou, une pression sur la touche pendant qu'on
    // clique sur le bouton de démarrage joindrait le même fil deux fois.
    std::mutex lifecycle;

    ClickPlan plan;
    IntervalFields interval;
    StartRefusal refusal{StartRefusal::None};

    platform::PointPicker picker;
    platform::CaptureOverlay overlay;
    void* mainWindow{nullptr};

    /// Une désignation a été demandée et n'est pas encore soldée.
    ///
    /// Sans ce drapeau, la boucle de récupération ne saurait pas distinguer
    /// « rien en cours » de « annulé à l'instant », et démonterait le crochet à
    /// chaque image.
    bool captureRequested{false};

    /// Mode de ciblage.
    ///
    /// Les points restent mémorisés quand il est désactivé : basculer sur
    /// « position actuelle » ne doit pas effacer un repérage qui a coûté du
    /// temps à saisir.
    bool useTargets{false};

    // Déclaré en dernier, donc détruit en premier : plus aucune touche ne peut
    // atteindre le moteur une fois la destruction commencée.
    std::unique_ptr<platform::HotkeyService> hotkeys;

    Impl() : governor{probe}, engine{sink, timer} {}
};

AppController::AppController() : m_impl{std::make_unique<Impl>()}
{
    m_impl->interval.milliseconds = kDefaultIntervalMilliseconds;

    // Les réglages d'ordonnancement doivent être posés sur le fil de cadence
    // lui-même ; la fabrique les y installe et le jeton les défait à l'arrêt.
    m_impl->engine.setThreadPreparation(
        [] { return std::static_pointer_cast<void>(std::make_shared<platform::ScopedThreadTuning>()); });

    // La source doit être immédiate : elle est consultée à chaque lot, sur le
    // fil de cadence. Elle ne fait donc que lire un atomique publié par le fil
    // de sondage, jamais sonder elle-même.
    m_impl->engine.setRateScaleSource([this] {
        return m_impl->governorEnabled.load(std::memory_order_relaxed) ? m_impl->governor.scale() : 1.0;
    });

    m_impl->hotkeys =
        std::make_unique<platform::HotkeyService>(platform::defaultPanicHotkey(), [this] { toggle(); });

    applyInterval();
}

AppController::~AppController() = default;

void AppController::start()
{
    const std::lock_guard guard{m_impl->lifecycle};

    m_impl->refusal =
        evaluateStart(m_impl->hotkeys->panicHotkeyActive(), m_impl->plan, m_impl->engine.isRunning());
    if (m_impl->refusal != StartRefusal::None)
    {
        return;
    }

    // Le gouverneur démarre avant le moteur : la première mesure doit être
    // disponible avant le premier lot, sinon la session part à pleine cadence
    // sur une cible qu'on n'a pas encore regardée.
    if (m_impl->governorEnabled.load(std::memory_order_relaxed))
    {
        m_impl->governor.start();
    }

    // Le plan transmis au moteur est une copie : en mode « position actuelle »
    // on lui retire les cibles sans les effacer de la saisie, pour que basculer
    // d'un mode à l'autre ne detruise pas un travail de reperage.
    ClickPlan effective = m_impl->plan;
    if (!m_impl->useTargets)
    {
        effective.targets.clear();
    }

    m_impl->engine.start(effective);
}

void AppController::stop()
{
    const std::lock_guard guard{m_impl->lifecycle};
    m_impl->engine.stop();
    m_impl->governor.stop();
}

void AppController::toggle()
{
    // Volontairement sans verrou ici : start et stop le prennent chacun, et
    // les tenir tous deux sous un même verrou n'apporterait rien qu'un risque
    // de le prendre deux fois.
    if (m_impl->engine.isRunning())
    {
        stop();
    }
    else
    {
        start();
    }
}

bool AppController::isRunning() const noexcept
{
    return m_impl->engine.isRunning();
}

StartRefusal AppController::lastRefusal() const noexcept
{
    return m_impl->refusal;
}

std::string AppController::refusalMessage() const
{
    switch (m_impl->refusal)
    {
    case StartRefusal::NoPanicHotkey:
        return "Aucun raccourci d'arret d'urgence : une autre application le detient. "
               "Choisissez-en un autre dans les reglages avant de lancer.";
    case StartRefusal::InvalidRate:
        return "L'intervalle est nul. Reglez une valeur superieure a zero.";
    case StartRefusal::AlreadyRunning:
        return "Une session tourne deja.";
    case StartRefusal::None:
        break;
    }

    return {};
}

EngineSnapshot AppController::snapshot() const noexcept
{
    return m_impl->engine.snapshot();
}

ClickPlan& AppController::plan() noexcept
{
    return m_impl->plan;
}

const ClickPlan& AppController::plan() const noexcept
{
    return m_impl->plan;
}

IntervalFields& AppController::interval() noexcept
{
    return m_impl->interval;
}

void AppController::applyInterval()
{
    const IntervalFields& fields = m_impl->interval;

    const Duration total = std::chrono::hours{fields.hours} + std::chrono::minutes{fields.minutes} +
                           std::chrono::seconds{fields.seconds} +
                           std::chrono::milliseconds{fields.milliseconds};

    m_impl->plan.clicksPerSecond = clicksPerSecondFromInterval(total);
}

Duration AppController::effectiveInterval() const noexcept
{
    // Repasse par le plafonnage du moteur : afficher l'intervalle demandé alors
    // que le moteur en appliquerait un autre serait un mensonge poli.
    const ClickPlan clamped = clampPlan(m_impl->plan, EngineConfig{}, m_impl->sink.maxBatchSize());
    return intervalFromClicksPerSecond(clamped.clicksPerSecond);
}

platform::Hotkey AppController::panicHotkey() const noexcept
{
    return m_impl->hotkeys->panicHotkey();
}

bool AppController::panicHotkeyActive() const noexcept
{
    return m_impl->hotkeys->panicHotkeyActive();
}

bool AppController::rebindPanicHotkey(platform::Hotkey hotkey)
{
    return m_impl->hotkeys->rebindPanicHotkey(hotkey);
}

void AppController::setMainWindow(void* window) noexcept
{
    m_impl->mainWindow = window;
}

bool AppController::beginPointCapture()
{
    if (!m_impl->picker.begin(m_impl->mainWindow))
    {
        return false;
    }

    m_impl->overlay.show(m_impl->picker.liveCursor());
    m_impl->captureRequested = true;
    return true;
}

void AppController::cancelPointCapture()
{
    m_impl->overlay.hide();
    m_impl->picker.cancel();
    m_impl->captureRequested = false;
}

bool AppController::capturingPoint() const noexcept
{
    return m_impl->picker.isActive();
}

ScreenPoint AppController::liveCursor() const noexcept
{
    return m_impl->picker.liveCursor();
}

void AppController::pollPointCapture()
{
    if (!m_impl->captureRequested)
    {
        return;
    }

    if (m_impl->picker.hasResult())
    {
        m_impl->plan.targets.push_back(m_impl->picker.takeResult());
        m_impl->useTargets = true;

        // La désignation est soldée : on démonte le crochet et on rend la
        // fenêtre. La laisser réduite obligerait à la retrouver dans la barre
        // des tâches après chaque point.
        cancelPointCapture();
        return;
    }

    // Le crochet s'est désactivé sans résultat : c'est une annulation au clic
    // droit. Il faut quand même le démonter et rendre la fenêtre, sinon
    // l'application resterait réduite sans que rien ne l'explique.
    if (!m_impl->picker.isActive())
    {
        cancelPointCapture();
        return;
    }

    // Le repère suit le curseur : c'est la seule chose visible pendant que la
    // fenêtre est réduite, et donc la seule qui dise à l'utilisateur ce qu'on
    // attend de lui.
    m_impl->overlay.moveTo(m_impl->picker.liveCursor());
}

void AppController::captureTarget()
{
    m_impl->plan.targets.push_back(platform::currentCursorPosition());
    m_impl->useTargets = true;
}

void AppController::setUseTargets(bool enabled) noexcept
{
    m_impl->useTargets = enabled;
}

bool AppController::useTargets() const noexcept
{
    return m_impl->useTargets;
}

void AppController::removeTarget(std::size_t index)
{
    std::vector<ScreenPoint>& targets = m_impl->plan.targets;
    if (index < targets.size())
    {
        targets.erase(targets.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void AppController::clearTargets()
{
    m_impl->plan.targets.clear();
}

void AppController::setGovernorEnabled(bool enabled) noexcept
{
    m_impl->governorEnabled.store(enabled, std::memory_order_relaxed);

    // Désactiver en cours de session arrête le sondage tout de suite : laisser
    // un fil mesurer ce que plus personne ne lit serait du travail pur perte.
    if (!enabled)
    {
        m_impl->governor.stop();
    }
    else if (m_impl->engine.isRunning())
    {
        m_impl->governor.start();
    }
}

bool AppController::governorEnabled() const noexcept
{
    return m_impl->governorEnabled.load(std::memory_order_relaxed);
}

double AppController::governorScale() const noexcept
{
    return m_impl->governorEnabled.load(std::memory_order_relaxed) ? m_impl->governor.scale() : 1.0;
}

Duration AppController::governorLatency() const noexcept
{
    return m_impl->governor.lastLatency();
}

bool AppController::targetHung() const noexcept
{
    return m_impl->governor.targetHung();
}

} // namespace deuca::ui
