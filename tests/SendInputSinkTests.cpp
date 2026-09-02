#include "platform/SendInputSink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <vector>

//
// AVERTISSEMENT — a lire avant d'ajouter un cas ici.
//
// Aucun test de ce fichier ne soumet un lot valide, et c'est deliberé. Une
// soumission reussie deplacerait le curseur de la machine qui execute la suite
// et cliquerait sur ce qui se trouve dessous : le poste d'un contributeur, ou
// le bureau d'un runner d'integration continue.
//
// On verifie donc les garde-fous, c'est-a-dire les chemins qui refusent avant
// d'atteindre le systeme. Le comportement d'injection reel se verifie au banc,
// sur une machine dediee, avec une fenetre receptrice qui horodate ce qu'elle
// recoit.
//

TEST_CASE("Le plafond de lot est respecte", "[platform][sink]")
{
    const deuca::platform::SendInputSink sink{64};
    REQUIRE(sink.maxBatchSize() == 64);
}

TEST_CASE("Le plafond de lot ne descend pas sous un clic complet", "[platform][sink]")
{
    // Un plafond de zero ou un rendrait tout lot impossible : il faut au moins
    // de quoi loger un appui et son relachement.
    const deuca::platform::SendInputSink zero{0};
    REQUIRE(zero.maxBatchSize() == 2);

    const deuca::platform::SendInputSink one{1};
    REQUIRE(one.maxBatchSize() == 2);
}

TEST_CASE("Un lot vide est refuse sans rien injecter", "[platform][sink]")
{
    deuca::platform::SendInputSink sink;

    REQUIRE(sink.submit({}) == 0);

    // Refus en amont, donc rien n'a atteint le systeme : ce n'est pas un
    // blocage par un autre processus.
    REQUIRE_FALSE(sink.wasBlocked());
}

TEST_CASE("Un lot plus grand que le plafond est refuse sans rien injecter", "[platform][sink]")
{
    deuca::platform::SendInputSink sink{2};

    const std::vector<deuca::ClickEvent> tooLarge(8);
    REQUIRE(sink.submit(tooLarge) == 0);
    REQUIRE_FALSE(sink.wasBlocked());
}

TEST_CASE("Le bureau virtuel n'est lu qu'en cas de deplacement", "[platform][sink]")
{
    const deuca::platform::SendInputSink sink;

    // Aucun lot soumis : la valeur reste celle par defaut, elle n'est pas lue
    // a la construction. Relire a chaque lot comportant un deplacement est ce
    // qui permet de suivre le branchement d'un ecran en cours de route.
    const deuca::VirtualDesktop desktop = sink.lastKnownDesktop();
    REQUIRE(desktop.width == 1);
    REQUIRE(desktop.height == 1);
}
