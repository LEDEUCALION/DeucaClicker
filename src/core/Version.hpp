#pragma once

#include <compare>
#include <cstdint>
#include <string>

namespace deuca
{

/// Version sémantique de la build en cours d'exécution.
///
/// Placée dans core/ plutôt que dans l'interface graphique pour que le lanceur
/// de tests, le banc de mesure et l'application annoncent tous le même numéro.
struct Version
{
    std::uint16_t major{};
    std::uint16_t minor{};
    std::uint16_t patch{};

    [[nodiscard]] std::string toString() const;

    friend auto operator<=>(const Version&, const Version&) = default;
};

/// La version depuis laquelle ce binaire a été compilé.
[[nodiscard]] Version currentVersion() noexcept;

/// Bandeau de build lisible, du type « DeucaClicker 0.1.0 ». Utilisé dans le
/// titre de la fenêtre, la boîte « à propos » et les rapports de mesure.
[[nodiscard]] std::string buildBanner();

} // namespace deuca
