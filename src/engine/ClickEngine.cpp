#include "engine/ClickEngine.hpp"

#include "core/ClickSequencer.hpp"
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

    // Un lot doit tenir en entier dans le puits : deux événements par clic, et
    // pas de lot tronqué qui laisserait un bouton enfoncé.
    const std::size_t maxClicks = std::max<std::size_t>(1, maxBatchSize / 2);
    plan.burstSize = std::min(plan.burstSize, maxClicks);

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

    m_running.store(true, std::memory_order_relaxed);

    while (!token.stop_requested())
    {
        // Homme mort : une session oubliée en marche s'arrête d'elle-même.
        if (m_config.maxRunDuration > Duration::zero() && Clock::now() - startedAt >= m_config.maxRunDuration)
        {
            break;
        }

        // Échéance avancée d'une période pleine plutôt que recalculée depuis
        // l'instant courant : cumuler le retard de chaque tour ferait dériver
        // la cadence au lieu de la tenir.
        deadline += period;
        waiter.waitUntil(deadline);

        if (token.stop_requested())
        {
            break;
        }

        const std::size_t written = sequencer.fillBurst(burst);
        if (written == 0)
        {
            break;
        }

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
