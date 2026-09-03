#include "ui/AppController.hpp"

#include "platform/Cursor.hpp"
#include "platform/HighResolutionTimer.hpp"
#include "platform/HotkeyService.hpp"
#include "platform/SendInputSink.hpp"
#include "platform/ThreadTuning.hpp"

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
    ClickEngine engine;

    // Protège les démarrages et arrêts. Le raccourci global agit depuis son
    // propre fil : sans ce verrou, une pression sur la touche pendant qu'on
    // clique sur le bouton de démarrage joindrait le même fil deux fois.
    std::mutex lifecycle;

    ClickPlan plan;
    IntervalFields interval;
    StartRefusal refusal{StartRefusal::None};

    // Déclaré en dernier, donc détruit en premier : plus aucune touche ne peut
    // atteindre le moteur une fois la destruction commencée.
    std::unique_ptr<platform::HotkeyService> hotkeys;

    Impl() : engine{sink, timer} {}
};

AppController::AppController() : m_impl{std::make_unique<Impl>()}
{
    m_impl->interval.milliseconds = kDefaultIntervalMilliseconds;

    // Les réglages d'ordonnancement doivent être posés sur le fil de cadence
    // lui-même ; la fabrique les y installe et le jeton les défait à l'arrêt.
    m_impl->engine.setThreadPreparation(
        [] { return std::static_pointer_cast<void>(std::make_shared<platform::ScopedThreadTuning>()); });

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

    m_impl->engine.start(m_impl->plan);
}

void AppController::stop()
{
    const std::lock_guard guard{m_impl->lifecycle};
    m_impl->engine.stop();
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

void AppController::captureTarget()
{
    m_impl->plan.targets.push_back(platform::currentCursorPosition());
}

void AppController::clearTargets()
{
    m_impl->plan.targets.clear();
}

} // namespace deuca::ui
