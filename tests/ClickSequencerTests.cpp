#include "core/ClickSequencer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>

using namespace std::chrono_literals;

TEST_CASE("Un lot compte deux evenements par clic", "[plan]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 1;
    REQUIRE(deuca::eventsPerBurst(plan) == 2);

    plan.burstSize = 10;
    REQUIRE(deuca::eventsPerBurst(plan) == 20);

    // Un lot de zero clic n'aurait pas de sens : on le ramene a un.
    plan.burstSize = 0;
    REQUIRE(deuca::eventsPerBurst(plan) == 2);
}

TEST_CASE("La periode de lot tient compte de la taille du lot", "[plan]")
{
    deuca::ClickPlan plan;

    plan.clicksPerSecond = 10.0;
    plan.burstSize = 1;
    REQUIRE(deuca::burstPeriod(plan) == deuca::Duration{100ms});

    // Dix clics groupes a mille clics par seconde : un lot toutes les dix
    // millisecondes, pas toutes les millisecondes.
    plan.clicksPerSecond = 1000.0;
    plan.burstSize = 10;
    REQUIRE(deuca::burstPeriod(plan) == deuca::Duration{10ms});
}

TEST_CASE("Une cadence nulle ou negative ne produit pas de periode", "[plan]")
{
    deuca::ClickPlan plan;

    plan.clicksPerSecond = 0.0;
    REQUIRE(deuca::burstPeriod(plan) == deuca::Duration::zero());

    plan.clicksPerSecond = -5.0;
    REQUIRE(deuca::burstPeriod(plan) == deuca::Duration::zero());
}

TEST_CASE("Un tampon trop petit fait echouer le lot en entier", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 3;

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 4> tooSmall{};

    // Refus net : un lot coupe entre l'appui et le relachement laisserait le
    // bouton enfonce.
    REQUIRE(sequencer.fillBurst(tooSmall) == 0);
}

TEST_CASE("Un clic produit un appui puis un relachement", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.button = deuca::MouseButton::Right;
    plan.burstSize = 1;

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 2> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 2);
    REQUIRE(buffer[0].action == deuca::ButtonAction::Press);
    REQUIRE(buffer[1].action == deuca::ButtonAction::Release);
    REQUIRE(buffer[0].button == deuca::MouseButton::Right);
    REQUIRE(buffer[1].button == deuca::MouseButton::Right);
}

TEST_CASE("Sans cible, aucun deplacement n'est demande", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 2;

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 4> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 4);
    for (const deuca::ClickEvent& event : buffer)
    {
        REQUIRE_FALSE(event.moveTo.has_value());
    }
}

TEST_CASE("Le relachement ne se deplace jamais", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 2;
    plan.targets = {{10, 20}, {30, 40}};

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 4> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 4);

    // Deplacer entre l'appui et le relachement produirait un glisser.
    REQUIRE(buffer[0].moveTo.has_value());
    REQUIRE_FALSE(buffer[1].moveTo.has_value());
    REQUIRE(buffer[2].moveTo.has_value());
    REQUIRE_FALSE(buffer[3].moveTo.has_value());
}

TEST_CASE("La rotation des cibles se poursuit d'un lot a l'autre", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 2;
    plan.targets = {{1, 1}, {2, 2}, {3, 3}};

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 4> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 4);
    REQUIRE(buffer[0].moveTo == deuca::ScreenPoint{1, 1});
    REQUIRE(buffer[2].moveTo == deuca::ScreenPoint{2, 2});

    // Sans etat conserve, un lot de deux clics sur trois cibles ne visiterait
    // jamais la troisieme.
    REQUIRE(sequencer.fillBurst(buffer) == 4);
    REQUIRE(buffer[0].moveTo == deuca::ScreenPoint{3, 3});
    REQUIRE(buffer[2].moveTo == deuca::ScreenPoint{1, 1});
}

TEST_CASE("La remise a zero ramene a la premiere cible", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 1;
    plan.targets = {{1, 1}, {2, 2}};

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 2> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 2);
    REQUIRE(buffer[0].moveTo == deuca::ScreenPoint{1, 1});

    sequencer.reset();

    REQUIRE(sequencer.fillBurst(buffer) == 2);
    REQUIRE(buffer[0].moveTo == deuca::ScreenPoint{1, 1});
}

TEST_CASE("Un intervalle se convertit en cadence", "[plan]")
{
    REQUIRE(deuca::clicksPerSecondFromInterval(deuca::Duration{100ms}) == 10.0);
    REQUIRE(deuca::clicksPerSecondFromInterval(deuca::Duration{1s}) == 1.0);
    REQUIRE(deuca::clicksPerSecondFromInterval(deuca::Duration{1ms}) == 1000.0);
}

TEST_CASE("Un intervalle nul ou negatif ne donne pas de cadence", "[plan]")
{
    REQUIRE(deuca::clicksPerSecondFromInterval(deuca::Duration::zero()) == 0.0);
    REQUIRE(deuca::clicksPerSecondFromInterval(deuca::Duration{-5ms}) == 0.0);
}

TEST_CASE("Une cadence se convertit en intervalle", "[plan]")
{
    REQUIRE(deuca::intervalFromClicksPerSecond(10.0) == deuca::Duration{100ms});
    REQUIRE(deuca::intervalFromClicksPerSecond(1000.0) == deuca::Duration{1ms});
}

TEST_CASE("Une cadence nulle ou negative ne donne pas d'intervalle", "[plan]")
{
    REQUIRE(deuca::intervalFromClicksPerSecond(0.0) == deuca::Duration::zero());
    REQUIRE(deuca::intervalFromClicksPerSecond(-3.0) == deuca::Duration::zero());
}

TEST_CASE("Les deux conversions sont reciproques", "[plan]")
{
    const deuca::Duration interval{40ms};
    const double rate = deuca::clicksPerSecondFromInterval(interval);

    REQUIRE(deuca::intervalFromClicksPerSecond(rate) == interval);
}

TEST_CASE("Un double clic produit deux couples par activation", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.style = deuca::ClickStyle::Double;
    plan.burstSize = 1;

    REQUIRE(deuca::pressesPerActivation(plan.style) == 2);
    REQUIRE(deuca::eventsPerBurst(plan) == 4);

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 4> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 4);
    REQUIRE(buffer[0].action == deuca::ButtonAction::Press);
    REQUIRE(buffer[1].action == deuca::ButtonAction::Release);
    REQUIRE(buffer[2].action == deuca::ButtonAction::Press);
    REQUIRE(buffer[3].action == deuca::ButtonAction::Release);
}

TEST_CASE("Un double clic tombe deux fois au meme endroit", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.style = deuca::ClickStyle::Double;
    plan.burstSize = 1;
    plan.targets = {{10, 20}, {30, 40}};

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 4> buffer{};

    REQUIRE(sequencer.fillBurst(buffer) == 4);

    // La cible avance une fois par activation, pas une fois par appui : sinon
    // les deux moities du double clic tomberaient a deux endroits differents et
    // le systeme n'y verrait plus un double clic.
    REQUIRE(buffer[0].moveTo == deuca::ScreenPoint{10, 20});
    REQUIRE(buffer[2].moveTo == deuca::ScreenPoint{10, 20});

    REQUIRE(sequencer.fillBurst(buffer) == 4);
    REQUIRE(buffer[0].moveTo == deuca::ScreenPoint{30, 40});
}

TEST_CASE("Le plafond d'activations reduit le dernier lot", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 8;

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 16> buffer{};

    // Trois activations demandees sur un lot de huit : six evenements, pas
    // seize. Depasser une limite saisie par l'utilisateur discredite l'outil.
    REQUIRE(sequencer.fillBurst(buffer, 3) == 6);
}

TEST_CASE("Un plafond nul ne produit aucun evenement", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 4;

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 8> buffer{};

    REQUIRE(sequencer.fillBurst(buffer, 0) == 0);
}

TEST_CASE("Le plafond ne rallonge jamais un lot", "[sequencer]")
{
    deuca::ClickPlan plan;
    plan.burstSize = 2;

    deuca::ClickSequencer sequencer{plan};
    std::array<deuca::ClickEvent, 8> buffer{};

    REQUIRE(sequencer.fillBurst(buffer, 100) == 4);
}
