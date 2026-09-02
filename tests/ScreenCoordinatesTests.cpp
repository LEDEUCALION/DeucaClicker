#include "core/ScreenCoordinates.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Une etendue degeneree ne divise pas par zero", "[coords]")
{
    REQUIRE(deuca::normalizeAxis(100, 0, 0) == 0);
    REQUIRE(deuca::normalizeAxis(100, 0, 1) == 0);
    REQUIRE(deuca::normalizeAxis(100, 0, -5) == 0);
}

TEST_CASE("Les bornes tombent juste sur l'echelle absolue", "[coords]")
{
    // Sur un ecran de 1920 pixels, le dernier pixel est 1919 et il doit valoir
    // exactement la borne haute, pas 65500.
    REQUIRE(deuca::normalizeAxis(0, 0, 1920) == 0);
    REQUIRE(deuca::normalizeAxis(1919, 0, 1920) == 65535);
}

TEST_CASE("Une origine negative est prise en compte", "[coords]")
{
    // Un second ecran place a gauche du principal a des abscisses negatives.
    // C'est l'erreur classique des outils qui supposent un bureau commencant
    // a zero : tout ce qui est a gauche devient inatteignable.
    REQUIRE(deuca::normalizeAxis(-1920, -1920, 3840) == 0);
    REQUIRE(deuca::normalizeAxis(1919, -1920, 3840) == 65535);
}

TEST_CASE("Les valeurs hors etendue sont bornees", "[coords]")
{
    REQUIRE(deuca::normalizeAxis(-500, 0, 1920) == 0);
    REQUIRE(deuca::normalizeAxis(9000, 0, 1920) == 65535);
}

TEST_CASE("La conversion d'un point traite les deux axes", "[coords]")
{
    const deuca::VirtualDesktop desktop{.origin = {}, .width = 1920, .height = 1080};
    const deuca::ScreenPoint corner = deuca::toAbsolute(deuca::ScreenPoint{1919, 1079}, desktop);

    REQUIRE(corner.x == 65535);
    REQUIRE(corner.y == 65535);

    const deuca::ScreenPoint origin = deuca::toAbsolute(deuca::ScreenPoint{0, 0}, desktop);
    REQUIRE(origin == deuca::ScreenPoint{0, 0});
}
