#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace deuca::platform
{

/// Touches de modification, combinables.
enum class Modifier : std::uint32_t
{
    None = 0,
    Alt = 1u << 0,
    Control = 1u << 1,
    Shift = 1u << 2,
    Windows = 1u << 3,
};

[[nodiscard]] constexpr Modifier operator|(Modifier lhs, Modifier rhs) noexcept
{
    return static_cast<Modifier>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr bool contains(Modifier set, Modifier flag) noexcept
{
    return (static_cast<std::uint32_t>(set) & static_cast<std::uint32_t>(flag)) != 0;
}

/// Une combinaison de touches.
///
/// Le code de touche est un code virtuel Windows. L'exposer tel quel plutôt que
/// de le traduire dans une énumération maison est un choix assumé : la table
/// des codes virtuels compte plus de cent entrées, et l'interface de saisie de
/// raccourci recevra de toute façon ce code brut du système.
struct Hotkey
{
    Modifier modifiers{Modifier::None};
    std::uint32_t virtualKey{0};

    friend bool operator==(const Hotkey&, const Hotkey&) = default;
};

/// Raccourci d'arrêt d'urgence par défaut : F8 seul.
///
/// Choisi parce qu'il n'entre en conflit ni avec les raccourcis usuels des
/// navigateurs ni avec ceux des jeux, et parce qu'il reste atteignable d'une
/// seule main dans l'urgence. Un raccourci de panique qui demande trois doigts
/// n'est pas un raccourci de panique.
[[nodiscard]] Hotkey defaultPanicHotkey() noexcept;

/// Associe des identifiants de raccourci à leurs actions.
///
/// Isolée de tout ce qui touche au système : la distribution d'un identifiant
/// vers la bonne action se vérifie ici sans fenêtre, sans fil d'exécution et
/// sans appuyer sur la moindre touche.
class HotkeyTable
{
public:
    using Callback = std::function<void()>;

    /// @return false si l'identifiant est déjà pris.
    bool add(int id, Callback callback);

    /// @return false si l'identifiant est inconnu.
    bool remove(int id);

    /// Exécute l'action associée.
    ///
    /// @return false si l'identifiant est inconnu, ce qui doit être traité
    ///         comme un défaut de synchronisation et non ignoré.
    bool dispatch(int id) const;

    [[nodiscard]] bool contains(int id) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::unordered_map<int, Callback> m_callbacks;
};

} // namespace deuca::platform
