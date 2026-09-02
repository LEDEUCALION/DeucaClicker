#include "core/ScreenCoordinates.hpp"

#include <algorithm>

namespace deuca
{
namespace
{

/// Borne haute de l'échelle absolue du flux d'entrée.
constexpr std::int64_t kAbsoluteMaximum = 65535;

} // namespace

std::int32_t normalizeAxis(std::int32_t value, std::int32_t origin, std::int32_t extent) noexcept
{
    if (extent <= 1)
    {
        return 0;
    }

    // Calcul intermédiaire sur 64 bits : le produit d'une coordonnée par 65535
    // dépasse la plage des entiers 32 bits dès quelques dizaines de milliers de
    // pixels, ce qu'un mur d'écrans atteint sans difficulté.
    const std::int64_t offset = static_cast<std::int64_t>(value) - origin;
    const std::int64_t scaled = offset * kAbsoluteMaximum / (static_cast<std::int64_t>(extent) - 1);

    return static_cast<std::int32_t>(std::clamp<std::int64_t>(scaled, 0, kAbsoluteMaximum));
}

ScreenPoint toAbsolute(ScreenPoint point, const VirtualDesktop& desktop) noexcept
{
    return ScreenPoint{.x = normalizeAxis(point.x, desktop.origin.x, desktop.width),
                       .y = normalizeAxis(point.y, desktop.origin.y, desktop.height)};
}

} // namespace deuca
