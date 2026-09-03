#include "engine/RateGovernor.hpp"

namespace deuca
{

RateGovernor::RateGovernor(ITargetProbe& probe, Config config) : m_probe{&probe}, m_config{config} {}

RateGovernor::~RateGovernor()
{
    stop();
}

void RateGovernor::start()
{
    stop();

    m_scale.store(1.0, std::memory_order_relaxed);
    m_lastLatency.store(0, std::memory_order_relaxed);
    m_targetHung.store(false, std::memory_order_relaxed);
    m_probeCount.store(0, std::memory_order_relaxed);

    m_worker = std::jthread{[this](std::stop_token token) { run(std::move(token)); }};
}

void RateGovernor::stop()
{
    if (m_worker.joinable())
    {
        m_worker.request_stop();
        m_worker.join();
    }

    m_running.store(false, std::memory_order_relaxed);

    // Le facteur revient à un : le laisser à sa dernière valeur ferait démarrer
    // la session suivante bridée par une mesure qui ne la concerne pas.
    m_scale.store(1.0, std::memory_order_relaxed);
}

bool RateGovernor::isRunning() const noexcept
{
    return m_running.load(std::memory_order_relaxed);
}

double RateGovernor::scale() const noexcept
{
    return m_scale.load(std::memory_order_relaxed);
}

Duration RateGovernor::lastLatency() const noexcept
{
    return Duration{m_lastLatency.load(std::memory_order_relaxed)};
}

bool RateGovernor::targetHung() const noexcept
{
    return m_targetHung.load(std::memory_order_relaxed);
}

std::uint64_t RateGovernor::probeCount() const noexcept
{
    return m_probeCount.load(std::memory_order_relaxed);
}

void RateGovernor::run(std::stop_token token)
{
    m_running.store(true, std::memory_order_relaxed);

    while (!token.stop_requested())
    {
        const ProbeResult result = m_probe->probe();

        const double updated = nextScale(m_scale.load(std::memory_order_relaxed), result, m_config.policy);
        m_scale.store(updated, std::memory_order_relaxed);

        if (result.valid)
        {
            m_lastLatency.store(result.latency.count(), std::memory_order_relaxed);
            m_targetHung.store(result.hung, std::memory_order_relaxed);
        }

        m_probeCount.fetch_add(1, std::memory_order_relaxed);

        // Attente ordinaire : ce fil n'a aucune exigence de précision, et lui
        // donner une attente haute résolution ne ferait que consommer sans
        // rien apporter.
        std::this_thread::sleep_for(m_config.probeInterval);
    }

    m_running.store(false, std::memory_order_relaxed);
}

} // namespace deuca
