#include "core/GovernorPolicy.hpp"
#include "engine/RateGovernor.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{

/// Sondeur simule : il rend la suite de mesures qu'on lui a preparee.
///
/// C'est ce qui rend la politique verifiable : on peut eprouver une cible qui
/// s'engorge puis se degage, sequence qu'on ne saurait pas provoquer a la
/// demande sur une vraie fenetre.
class ScriptedProbe final : public deuca::ITargetProbe
{
public:
    explicit ScriptedProbe(deuca::ProbeResult fixed) noexcept : m_fixed{fixed} {}

    [[nodiscard]] deuca::ProbeResult probe() override
    {
        m_calls.fetch_add(1);
        return m_fixed;
    }

    [[nodiscard]] std::size_t calls() const noexcept { return m_calls.load(); }

private:
    deuca::ProbeResult m_fixed;
    std::atomic<std::size_t> m_calls{0};
};

[[nodiscard]] deuca::ProbeResult healthy()
{
    return deuca::ProbeResult{.latency = deuca::Duration{1ms}, .hung = false, .valid = true};
}

[[nodiscard]] deuca::ProbeResult congested()
{
    return deuca::ProbeResult{.latency = deuca::Duration{200ms}, .hung = false, .valid = true};
}

} // namespace

TEST_CASE("Une cible saine laisse la cadence remonter", "[governor]")
{
    const deuca::GovernorConfig config;

    const double raised = deuca::nextScale(0.5, healthy(), config);
    REQUIRE(raised > 0.5);
    REQUIRE(raised <= 1.0);
}

TEST_CASE("La remontee ne depasse jamais la cadence demandee", "[governor]")
{
    const deuca::GovernorConfig config;

    REQUIRE(deuca::nextScale(1.0, healthy(), config) == 1.0);
    REQUIRE(deuca::nextScale(0.99, healthy(), config) == 1.0);
}

TEST_CASE("Une cible engorgee fait reculer la cadence", "[governor]")
{
    const deuca::GovernorConfig config;

    // Decroissance multiplicative : on recule vite, parce qu'on ignore ou est
    // la limite et que tergiverser laisse la file de la cible s'engorger.
    REQUIRE(deuca::nextScale(1.0, congested(), config) == 0.5);
    REQUIRE(deuca::nextScale(0.5, congested(), config) == 0.25);
}

TEST_CASE("Le recul s'arrete au plancher", "[governor]")
{
    deuca::GovernorConfig config;
    config.minimumScale = 0.1;

    REQUIRE(deuca::nextScale(0.1, congested(), config) == 0.1);
    REQUIRE(deuca::nextScale(0.15, congested(), config) == 0.1);
}

TEST_CASE("Une cible figee descend directement au plancher", "[governor]")
{
    deuca::GovernorConfig config;
    config.minimumScale = 0.05;

    const deuca::ProbeResult hung{.latency = deuca::Duration{500ms}, .hung = true, .valid = true};

    // Ralentir progressivement n'aurait aucun sens : une application figee ne
    // traite plus rien du tout.
    REQUIRE(deuca::nextScale(1.0, hung, config) == 0.05);
}

TEST_CASE("Un sondage impossible ne fait rien bouger", "[governor]")
{
    const deuca::GovernorConfig config;
    const deuca::ProbeResult unknown{};

    // Accelerer sur une absence d'information reviendrait a supposer que tout
    // va bien parce qu'on n'a pas regarde.
    REQUIRE(deuca::nextScale(0.4, unknown, config) == 0.4);
}

TEST_CASE("Un facteur plus petit allonge la periode", "[governor]")
{
    REQUIRE(deuca::scaledPeriod(deuca::Duration{100ms}, 1.0) == deuca::Duration{100ms});
    REQUIRE(deuca::scaledPeriod(deuca::Duration{100ms}, 0.5) == deuca::Duration{200ms});
    REQUIRE(deuca::scaledPeriod(deuca::Duration{100ms}, 0.25) == deuca::Duration{400ms});
}

TEST_CASE("Un facteur nul ne divise pas par zero", "[governor]")
{
    const deuca::Duration slowed = deuca::scaledPeriod(deuca::Duration{10ms}, 0.0);

    // Une cadence tres lente est un defaut visible et corrigible ; une division
    // par zero est un plantage.
    REQUIRE(slowed > deuca::Duration{10ms});
    REQUIRE(slowed < deuca::Duration{60s});
}

TEST_CASE("Le gouverneur sonde puis rend le facteur a l'arret", "[governor]")
{
    ScriptedProbe probe{congested()};

    deuca::RateGovernor::Config config;
    config.probeInterval = deuca::Duration{5ms};

    deuca::RateGovernor governor{probe, config};
    governor.start();

    const deuca::Timestamp limit = deuca::Clock::now() + deuca::Duration{3s};
    while (governor.scale() >= 1.0 && deuca::Clock::now() < limit)
    {
        std::this_thread::sleep_for(1ms);
    }

    REQUIRE(probe.calls() > 0);
    REQUIRE(governor.scale() < 1.0);

    governor.stop();

    // Le facteur revient a un : le laisser a sa derniere valeur ferait demarrer
    // la session suivante bridee par une mesure qui ne la concerne pas.
    REQUIRE(governor.scale() == 1.0);
    REQUIRE_FALSE(governor.isRunning());
}

TEST_CASE("La destruction arrete le fil de sondage", "[governor]")
{
    ScriptedProbe probe{healthy()};

    {
        deuca::RateGovernor::Config config;
        config.probeInterval = deuca::Duration{5ms};

        deuca::RateGovernor governor{probe, config};
        governor.start();

        const deuca::Timestamp limit = deuca::Clock::now() + deuca::Duration{2s};
        while (probe.calls() == 0 && deuca::Clock::now() < limit)
        {
            std::this_thread::sleep_for(1ms);
        }
        REQUIRE(probe.calls() > 0);
    }

    SUCCEED();
}
