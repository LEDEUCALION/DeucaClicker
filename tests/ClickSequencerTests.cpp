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
