#include "core/PrecisionWaiter.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace std::chrono_literals;

namespace
{

/// Dormeur simulé : il n'attend rien, il note ce qu'on lui a demandé.
///
/// C'est tout l'intérêt d'avoir abstrait l'attente bloquante. La politique de
/// cadence se vérifie ici en quelques microsecondes, sans primitive système,
/// sans session graphique, et sans dépendre de la charge de la machine.
class FakeSleeper final : public deuca::IBlockingSleeper
{
public:
    explicit FakeSleeper(deuca::Duration granularity) noexcept : m_granularity{granularity} {}

    [[nodiscard]] deuca::Duration granularity() const noexcept override { return m_granularity; }

    void sleepFor(deuca::Duration duration) override { m_calls.push_back(duration); }

    [[nodiscard]] const std::vector<deuca::Duration>& calls() const noexcept { return m_calls; }

private:
    deuca::Duration m_granularity;
    std::vector<deuca::Duration> m_calls;
};

} // namespace

TEST_CASE("Part bloquante nulle quand il ne reste plus de temps", "[waiting]")
{
    // Un temps restant négatif veut dire que l'échéance est derrière nous.
    // Dormir davantage ne rattraperait rien, cela ne ferait qu'aggraver le
    // retard.
    REQUIRE(deuca::blockingPortion(-5ms, 300us) == deuca::Duration::zero());
    REQUIRE(deuca::blockingPortion(deuca::Duration::zero(), 300us) == deuca::Duration::zero());
}

TEST_CASE("Part bloquante nulle quand le temps restant tient dans la marge", "[waiting]")
{
    REQUIRE(deuca::blockingPortion(300us, 300us) == deuca::Duration::zero());
    REQUIRE(deuca::blockingPortion(200us, 300us) == deuca::Duration::zero());
}

TEST_CASE("La part bloquante retranche la marge du temps restant", "[waiting]")
{
    REQUIRE(deuca::blockingPortion(10ms, 300us) == deuca::Duration{9700us});
}

TEST_CASE("La marge se recale sur un dormeur plus grossier", "[waiting]")
{
    // Le cas qui compte : bloquer jusqu'à « échéance moins 300 µs » avec un
    // dormeur au tick système reviendrait à se réveiller 15 ms trop tard une
    // fois sur deux.
    REQUIRE(deuca::effectiveSpinBudget(300us, 15625us) == deuca::Duration{15625us});
}

TEST_CASE("Une marge plus large que le dormeur ne bouge pas", "[waiting]")
{
    REQUIRE(deuca::effectiveSpinBudget(2ms, 500us) == deuca::Duration{2ms});
}

TEST_CASE("Aucun appel au dormeur quand le moment est atteint", "[waiting]")
{
    FakeSleeper sleeper{500us};
    deuca::PrecisionWaiter waiter{sleeper};

    const deuca::Timestamp deadline = deuca::Clock::now() - 1ms;
    const deuca::Timestamp wokeAt = waiter.waitUntil(deadline);

    REQUIRE(sleeper.calls().empty());
    REQUIRE(wokeAt >= deadline);
}

TEST_CASE("Aucun appel au dormeur pour une attente plus courte que la marge", "[waiting]")
{
    FakeSleeper sleeper{500us};
    deuca::PrecisionWaiter waiter{sleeper, deuca::PrecisionWaiter::Policy{.spinBudget = 1ms}};

    const deuca::Timestamp deadline = deuca::Clock::now() + 200us;
    const deuca::Timestamp wokeAt = waiter.waitUntil(deadline);

    REQUIRE(sleeper.calls().empty());
    REQUIRE(wokeAt >= deadline);
}

TEST_CASE("Une attente longue passe d'abord par le dormeur", "[waiting]")
{
    FakeSleeper sleeper{500us};
    deuca::PrecisionWaiter waiter{sleeper, deuca::PrecisionWaiter::Policy{.spinBudget = 1ms}};

    const deuca::Timestamp deadline = deuca::Clock::now() + 4ms;
    const deuca::Timestamp wokeAt = waiter.waitUntil(deadline);

    // Bornes volontairement larges : le dormeur simulé ne dort pas, donc
    // l'attente se termine en actif. On vérifie la décision, pas la durée
    // exacte, qui dépendrait de la charge du runner.
    REQUIRE(sleeper.calls().size() == 1);
    REQUIRE(sleeper.calls().front() > deuca::Duration::zero());
    REQUIRE(sleeper.calls().front() <= deuca::Duration{4ms});
    REQUIRE(wokeAt >= deadline);
}

TEST_CASE("La marge retenue est visible depuis l'exterieur", "[waiting]")
{
    // Dormeur au tick systeme : la marge remonte a sa granularite.
    FakeSleeper coarse{15625us};
    REQUIRE(deuca::PrecisionWaiter{coarse}.spinBudget() == deuca::Duration{15625us});

    // Dormeur plus fin que la marge demandee : la marge est gardee telle
    // quelle. Noter au passage qu'un timer haute resolution annonce 500 us,
    // donc la marge par defaut de 300 us sera relevee a 500 us en pratique.
    FakeSleeper fine{100us};
    REQUIRE(deuca::PrecisionWaiter{fine}.spinBudget() == deuca::Duration{300us});
}
