#include "core/PrecisionWaiter.hpp"
#include "platform/HighResolutionTimer.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

//
// Ces cas touchent une vraie primitive Windows, mais ils ne demandent ni
// fenêtre ni session interactive : ils tournent tels quels sur un runner.
//
// Les bornes hautes sont volontairement larges. Ce qui est vérifié ici, c'est
// que l'attente bloque réellement — la mesure fine de la gigue est le travail
// du banc, pas celui d'une suite de tests qui doit rester verte sous charge.
//

TEST_CASE("Le timer attendable est disponible", "[platform][timer]")
{
    const deuca::platform::HighResolutionTimer timer;
    REQUIRE(timer.isValid());
}

TEST_CASE("La granularite annoncee suit le mode obtenu", "[platform][timer]")
{
    const deuca::platform::HighResolutionTimer timer;

    if (timer.isHighResolution())
    {
        REQUIRE(timer.granularity() == deuca::Duration{500us});
    }
    else
    {
        // Repli sur le tick systeme, avant Windows 10 1803.
        REQUIRE(timer.granularity() == deuca::Duration{15625us});
    }
}

TEST_CASE("Une duree nulle ou negative rend la main aussitot", "[platform][timer]")
{
    deuca::platform::HighResolutionTimer timer;

    const deuca::Timestamp before = deuca::Clock::now();
    timer.sleepFor(deuca::Duration::zero());
    timer.sleepFor(-5ms);
    const deuca::Duration elapsed = deuca::Clock::now() - before;

    REQUIRE(elapsed < deuca::Duration{1ms});
}

TEST_CASE("L'attente bloque effectivement", "[platform][timer]")
{
    deuca::platform::HighResolutionTimer timer;

    const deuca::Timestamp before = deuca::Clock::now();
    timer.sleepFor(5ms);
    const deuca::Duration elapsed = deuca::Clock::now() - before;

    // La troncature en unites de 100 ns fait dormir un poil moins que demande,
    // et c'est voulu : un reveil precoce se rattrape en attente active, un
    // reveil tardif est une echeance manquee.
    REQUIRE(elapsed > deuca::Duration{1ms});
    REQUIRE(elapsed < deuca::Duration{500ms});
}

TEST_CASE("Le waiter atteint son echeance avec le vrai timer", "[platform][timer]")
{
    deuca::platform::HighResolutionTimer timer;
    deuca::PrecisionWaiter waiter{timer};

    const deuca::Timestamp deadline = deuca::Clock::now() + 8ms;
    const deuca::Timestamp wokeAt = waiter.waitUntil(deadline);

    REQUIRE(wokeAt >= deadline);
    REQUIRE(wokeAt - deadline < deuca::Duration{50ms});
}
