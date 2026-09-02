#include "platform/Hotkey.hpp"
#include "platform/HotkeyService.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>

//
// La distribution d'un identifiant vers son action se verifie ici sans fenetre
// ni fil d'execution. Ce qui ne peut pas etre teste, c'est l'appui reel sur une
// touche : declencher un WM_HOTKEY depuis la suite reviendrait a tester
// l'echafaudage plutot que le service, et exposer le handle de la fenetre pour
// y parvenir abimerait la conception pour le seul confort des tests.
//

TEST_CASE("Les modificateurs se combinent et se relisent", "[hotkey]")
{
    using deuca::platform::Modifier;

    const Modifier combination = Modifier::Control | Modifier::Shift;

    REQUIRE(deuca::platform::contains(combination, Modifier::Control));
    REQUIRE(deuca::platform::contains(combination, Modifier::Shift));
    REQUIRE_FALSE(deuca::platform::contains(combination, Modifier::Alt));
    REQUIRE_FALSE(deuca::platform::contains(combination, Modifier::Windows));
}

TEST_CASE("Aucun modificateur ne contient rien", "[hotkey]")
{
    using deuca::platform::Modifier;

    REQUIRE_FALSE(deuca::platform::contains(Modifier::None, Modifier::Alt));
    REQUIRE_FALSE(deuca::platform::contains(Modifier::None, Modifier::Control));
}

TEST_CASE("La table refuse un identifiant deja pris", "[hotkey]")
{
    deuca::platform::HotkeyTable table;

    REQUIRE(table.add(1, [] {}));
    REQUIRE_FALSE(table.add(1, [] {}));
    REQUIRE(table.size() == 1);
}

TEST_CASE("La table refuse une action vide", "[hotkey]")
{
    deuca::platform::HotkeyTable table;

    REQUIRE_FALSE(table.add(1, nullptr));
    REQUIRE(table.size() == 0);
}

TEST_CASE("La table execute l'action associee", "[hotkey]")
{
    deuca::platform::HotkeyTable table;

    int fired = 0;
    REQUIRE(table.add(7, [&fired] { ++fired; }));

    REQUIRE(table.dispatch(7));
    REQUIRE(fired == 1);
}

TEST_CASE("Un identifiant inconnu est signale et non ignore", "[hotkey]")
{
    const deuca::platform::HotkeyTable table;

    // Un faux dans ce cas veut dire que le systeme connait un raccourci que
    // nous avons oublie : c'est un defaut de synchronisation, pas un
    // non-evenement.
    REQUIRE_FALSE(table.dispatch(42));
}

TEST_CASE("Le retrait d'une action fonctionne une seule fois", "[hotkey]")
{
    deuca::platform::HotkeyTable table;
    REQUIRE(table.add(3, [] {}));

    REQUIRE(table.remove(3));
    REQUIRE_FALSE(table.remove(3));
    REQUIRE_FALSE(table.contains(3));
}

TEST_CASE("Le raccourci de panique par defaut est atteignable d'une main", "[hotkey]")
{
    const deuca::platform::Hotkey panic = deuca::platform::defaultPanicHotkey();

    // Un raccourci d'urgence qui demande trois doigts n'est pas un raccourci
    // d'urgence.
    REQUIRE(panic.modifiers == deuca::platform::Modifier::None);
    REQUIRE(panic.virtualKey != 0);
}

TEST_CASE("Le service demarre et tient sa fenetre", "[platform][hotkey]")
{
    std::atomic<int> panics{0};
    const deuca::platform::HotkeyService service{deuca::platform::defaultPanicHotkey(),
                                                 [&panics] { panics.fetch_add(1); }};

    REQUIRE(service.isRunning());
}

TEST_CASE("Le raccourci de panique n'est pas retirable", "[platform][hotkey]")
{
    deuca::platform::HotkeyService service{deuca::platform::defaultPanicHotkey(), [] {}};

    // Aucune methode n'expose son identifiant, et les valeurs qui pourraient le
    // designer sont refusees. Un arret d'urgence desactivable n'en est pas un.
    REQUIRE_FALSE(service.unregisterHotkey(0));
    REQUIRE_FALSE(service.unregisterHotkey(1));
    REQUIRE_FALSE(service.unregisterHotkey(-5));
}

TEST_CASE("Un raccourci inconnu ne se retire pas", "[platform][hotkey]")
{
    deuca::platform::HotkeyService service{deuca::platform::defaultPanicHotkey(), [] {}};

    REQUIRE_FALSE(service.unregisterHotkey(9999));
}

TEST_CASE("Une action vide est refusee a l'enregistrement", "[platform][hotkey]")
{
    deuca::platform::HotkeyService service{deuca::platform::defaultPanicHotkey(), [] {}};

    const deuca::platform::Hotkey hotkey{.modifiers = deuca::platform::Modifier::Control, .virtualKey = 0x71};
    REQUIRE_FALSE(service.registerHotkey(hotkey, nullptr).has_value());
}
