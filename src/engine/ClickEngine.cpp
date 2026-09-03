#include "engine/ClickEngine.hpp"

#include "core/ClickSequencer.hpp"
#include "core/GovernorPolicy.hpp"
#include "core/PrecisionWaiter.hpp"

#include <algorithm>
#include <vector>

namespace deuca
{

ClickPlan clampPlan(ClickPlan plan, const EngineConfig& config, std::size_t maxBatchSize) noexcept
{
    if (config.maxClicksPerSecond > 0.0)
    {
        plan.clicksPerSecond = std::min(plan.clicksPerSecond, config.maxClicksPerSecond);
    }

    plan.burstSize = std::max<std::size_t>(1, plan.burstSize);

    // Un lot doit tenir en entier dans le puits, sans quoi il serait tronqué et
    // laisserait un bouton enfoncé. Une activation coûte deux événements par
    // couple appui-relâchement, et un double-clic en compte deux.
    const std::size_t perActivation = pressesPerActivation(plan.style) * 2;
    const std::size_t maxActivations = std::max<std::size_t>(1, maxBatchSize / perActivation);
    plan.burstSize = std::min(plan.burstSize, maxActivations);

    return plan;
}

StartRefusal evaluateStart(bool panicHotkeyActive, const ClickPlan& plan, bool alreadyRunning) noexcept
{
    if (alreadyRunning)
    {
        return StartRefusal::AlreadyRunning;
    }

    // Vérifié avant la cadence : c'est la condition de sûreté, elle prime sur
    // la condition de bon sens.
    if (!panicHotkeyActive)
    {
        return StartRefusal::NoPanicHotkey;
    }

    if (burstPeriod(plan) == Duration::zero())
    {
        return StartRefusal::InvalidRate;
    }

    return StartRefusal::None;
}

ClickEngine::ClickEngine(IInputSink& sink, IBlockingSleeper& sleeper, EngineConfig config)
    : m_sink{&sink}, m_sleeper{&sleeper}, m_config{config}
{
}

ClickEngine::~ClickEngine()
{
    stop();
}

void ClickEngine::setThreadPreparation(ThreadPreparation preparation)
{
    m_preparation = std::move(preparation);
}

void ClickEngine::setRateScaleSource(std::function<double()> source)
{
    m_rateScaleSource = std::move(source);
}

void ClickEngine::start(ClickPlan plan)
{
    // Arrêt inconditionnel d'abord : deux boucles concurrentes se disputeraient
    // le puits et produiraient un entrelacement d'appuis et de relâchements que
    // personne ne saurait démêler.
    stop();

    const ClickPlan clamped = clampPlan(std::move(plan), m_config, m_sink->maxBatchSize());
    if (burstPeriod(clamped) == Duration::zero())
    {
        // Cadence nulle ou négative : le plan est inexploitable. Refuser de
        // démarrer vaut mieux que tourner à vide en laissant croire au
        // contraire.
        return;
    }

    m_burstsSubmitted.store(0, std::memory_order_relaxed);
    m_clicksEmitted.store(0, std::memory_order_relaxed);
    m_eventsRejected.store(0, std::memory_order_relaxed);
    m_blockedSubmissions.store(0, std::memory_order_relaxed);

    m_worker = std::jthread{[this, clamped](std::stop_token token) { run(std::move(token), clamped); }};
}

void ClickEngine::stop()
{
    if (m_worker.joinable())
    {
        m_worker.request_stop();
        m_worker.join();
    }

    m_running.store(false, std::memory_order_relaxed);
}

bool ClickEngine::isRunning() const noexcept
{
    return m_running.load(std::memory_order_relaxed);
}

EngineSnapshot ClickEngine::snapshot() const noexcept
{
    return EngineSnapshot{
        .burstsSubmitted = m_burstsSubmitted.load(std::memory_order_relaxed),
        .clicksEmitted = m_clicksEmitted.load(std::memory_order_relaxed),
        .eventsRejected = m_eventsRejected.load(std::memory_order_relaxed),
        .blockedSubmissions = m_blockedSubmissions.load(std::memory_order_relaxed),
        .running = m_running.load(std::memory_order_relaxed),
    };
}

void ClickEngine::run(std::stop_token token, ClickPlan plan)
{
    // Le jeton de préparation vit exactement le temps de ce fil d'exécution :
    // les réglages qu'il porte sont posés ici et défaits en sortant, y compris
    // si la boucle se termine par une exception.
    const std::shared_ptr<void> preparation = m_preparation ? m_preparation() : nullptr;

    PrecisionWaiter waiter{*m_sleeper, PrecisionWaiter::Policy{.spinBudget = m_config.spinBudget}};
    ClickSequencer sequencer{plan};

    // Tampon dimensionné une fois pour toutes, avant le premier tour : la
    // boucle ne doit rien allouer au moment précis où elle vise une échéance.
    std::vector<ClickEvent> burst(sequencer.eventsPerBurst());

    const Duration period = burstPeriod(plan);
    const Timestamp startedAt = Clock::now();
    Timestamp deadline = startedAt;

    // Nombre d'événements qu'une activation occupe, pour retrouver le compte
    // d'activations à partir du nombre d'événements écrits.
    const std::size_t eventsPerActivation = pressesPerActivation(plan.style) * 2;
    std::uint64_t emitted = 0;

    m_running.store(true, std::memory_order_relaxed);

    while (!token.stop_requested())
    {
        // Limite de répétition atteinte : la session s'arrête d'elle-même.
        if (plan.repeatLimit > 0 && emitted >= plan.repeatLimit)
        {
            break;
        }

        // Homme mort : une session oubliée en marche s'arrête d'elle-même.
        if (m_config.maxRunDuration > Duration::zero() && Clock::now() - startedAt >= m_config.maxRunDuration)
        {
            break;
        }

        // Le facteur est relu à chaque tour : c'est ce qui permet à
        // l'asservissement d'agir sur une session déjà lancée. La lecture doit
        // rester immédiate, le contrat de la source le dit.
        const double scale = m_rateScaleSource ? m_rateScaleSource() : 1.0;

        // Échéance avancée d'une période pleine plutôt que recalculée depuis
        // l'instant courant : cumuler le retard de chaque tour ferait dériver
        // la cadence au lieu de la tenir.
        deadline += scaledPeriod(period, scale);
        waiter.waitUntil(deadline);

        if (token.stop_requested())
        {
            break;
        }

        // Dernier lot d'une répétition limitée : on demande au séquenceur de ne
        // produire que ce qui reste, plutôt que de dépasser en silence une
        // limite que l'utilisateur a saisie.
        std::size_t allowed = ClickSequencer::kNoLimit;
        if (plan.repeatLimit > 0)
        {
            const std::uint64_t remaining = plan.repeatLimit - emitted;
            allowed = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, plan.burstSize));
        }

        const std::size_t written = sequencer.fillBurst(burst, allowed);
        if (written == 0)
        {
            break;
        }

        emitted += written / eventsPerActivation;

        const std::size_t accepted = m_sink->submit(std::span<const ClickEvent>{burst.data(), written});

        m_burstsSubmitted.fetch_add(1, std::memory_order_relaxed);
        m_clicksEmitted.fetch_add(accepted / 2, std::memory_order_relaxed);

        if (accepted < written)
        {
            m_eventsRejected.fetch_add(written - accepted, std::memory_order_relaxed);
        }

        if (accepted == 0)
        {
            m_blockedSubmissions.fetch_add(1, std::memory_order_relaxed);
        }
    }

    m_running.store(false, std::memory_order_relaxed);
}

} // namespace deuca
