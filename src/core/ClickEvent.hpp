#pragma once

#include "core/ScreenCoordinates.hpp"

#include <optional>

namespace deuca
{

enum class MouseButton
{
    Left,
    Right,
    Middle,
};

enum class ButtonAction
{
    Press,
    Release,
};

/// Un événement destiné au flux d'entrée.
///
/// Le déplacement est porté par l'événement lui-même plutôt que par un type
/// séparé : un appui sur une cible se décrit alors en une seule entrée du
/// tampon, et l'ordre déplacement-puis-appui ne peut pas être inversé par
/// erreur à l'assemblage du lot.
///
/// Un relâchement ne se déplace jamais : bouger le curseur entre l'appui et le
/// relâchement produit un glisser, pas un clic.
struct ClickEvent
{
    std::optional<ScreenPoint> moveTo{};
    MouseButton button{MouseButton::Left};
    ButtonAction action{ButtonAction::Press};
};

} // namespace deuca
