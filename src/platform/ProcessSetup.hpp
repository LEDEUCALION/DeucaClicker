#pragma once

namespace deuca::platform
{

/// Déclare le processus « per-monitor DPI aware » (V2).
///
/// Doit s'exécuter avant la création de la moindre fenêtre : Windows fige le
/// contexte de prise en charge du DPI à la première utilisation et ignore
/// silencieusement les changements ultérieurs. Sans cela, le bureau compose
/// notre fenêtre à partir d'une image mise à l'échelle — l'interface devient
/// floue et, surtout, les coordonnées du curseur qu'on nous transmet ne
/// correspondent plus à ce que l'utilisateur voit à l'écran.
///
/// @return false si le contexte avait déjà été fixé par un manifeste ou par un
///         appel précédent. Ce n'est pas une erreur, l'appelant peut l'ignorer.
bool enablePerMonitorDpiAwareness() noexcept;

} // namespace deuca::platform
