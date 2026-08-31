#include "core/Version.hpp"

#include <catch2/catch_test_macros.hpp>

//
// Les noms de cas sont volontairement écrits sans accent.
//
// CTest les repasse au binaire de test comme filtre en ligne de commande, et
// la page de codes de la console mutile les caractères non ASCII au passage :
// le filtre ne correspond alors plus à rien et le test est compté en échec
// alors qu'il n'a jamais été exécuté. Les commentaires et les messages, eux,
// gardent leurs accents — seuls les noms transitent par la ligne de commande.
//

TEST_CASE("Formatage de la version en semver", "[version]")
{
    const deuca::Version version{1, 2, 3};
    REQUIRE(version.toString() == "1.2.3");
}

TEST_CASE("Comparaison des versions par ordre d'importance", "[version]")
{
    REQUIRE(deuca::Version{0, 2, 0} > deuca::Version{0, 1, 9});
    REQUIRE(deuca::Version{1, 0, 0} > deuca::Version{0, 9, 9});
}

TEST_CASE("La build annonce une version non nulle", "[version]")
{
    // Garde-fou contre des définitions CMake qui n'atteindraient pas l'unité de
    // traduction : tous les champs resteraient alors à zéro sans rien signaler.
    REQUIRE(deuca::currentVersion() > deuca::Version{0, 0, 0});
    REQUIRE(deuca::buildBanner().starts_with("DeucaClicker "));
}
