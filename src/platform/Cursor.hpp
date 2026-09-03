#pragma once

#include "core/ScreenCoordinates.hpp"

namespace deuca::platform
{

/// Position courante du curseur, en pixels du bureau virtuel.
///
/// Sert à capturer une cible : l'utilisateur place sa souris où il veut, puis
/// demande à l'enregistrer. C'est plus fiable que de lui faire saisir des
/// coordonnées à la main, et cela évite d'avoir à capturer la souris pour un
/// mode de sélection dédié.
///
/// @return l'origine du bureau si le système refuse de répondre, ce qui
///         n'arrive que lorsque le bureau courant est verrouillé.
[[nodiscard]] ScreenPoint currentCursorPosition() noexcept;

} // namespace deuca::platform
