#pragma once

#include <cstdint>

namespace deuca
{

/// Un point du bureau, en pixels.
struct ScreenPoint
{
    std::int32_t x{};
    std::int32_t y{};

    friend bool operator==(const ScreenPoint&, const ScreenPoint&) = default;
};

/// Étendue du bureau virtuel : l'union rectangulaire de tous les écrans.
///
/// L'origine peut être négative — un écran placé à gauche du principal a des
/// abscisses négatives. C'est la source d'erreur classique des outils qui
/// supposent que le bureau commence à zéro et qui ratent tout ce qui se trouve
/// sur le second écran.
struct VirtualDesktop
{
    ScreenPoint origin{};
    std::int32_t width{1};
    std::int32_t height{1};
};

/// Convertit une coordonnée en valeur absolue normalisée sur 0-65535.
///
/// C'est l'échelle qu'attend le flux d'entrée absolu, indépendamment de la
/// résolution réelle. La borne haute correspond au dernier pixel, d'où le
/// « étendue moins un » : sur un écran de 1920 pixels, l'abscisse 1919 vaut
/// 65535 et non 65500.
///
/// @return zéro si l'étendue est dégénérée, plutôt qu'une division par zéro.
[[nodiscard]] std::int32_t normalizeAxis(std::int32_t value, std::int32_t origin,
                                         std::int32_t extent) noexcept;

/// Convertit un point du bureau en coordonnées absolues normalisées.
[[nodiscard]] ScreenPoint toAbsolute(ScreenPoint point, const VirtualDesktop& desktop) noexcept;

} // namespace deuca
