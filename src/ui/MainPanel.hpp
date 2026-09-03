#pragma once

namespace deuca::ui
{

class AppController;

/// État propre à l'affichage, sans effet sur le comportement.
///
/// Sorti de la fonction de dessin plutôt que confié à des variables statiques :
/// une fonction de dessin qui garde son propre état est indébogable, et rendrait
/// impossible l'ouverture d'une seconde fenêtre le jour où on en voudra une.
struct PanelState
{
    /// Combinaison en cours de saisie dans les réglages, pas encore appliquée.
    /// Sept correspond à F8, le raccourci par défaut.
    int pendingFunctionKeyIndex{7};
    bool pendingControl{false};
    bool pendingAlt{false};
    bool pendingShift{false};

    /// Dernier échec de reconfiguration, affiché jusqu'à la tentative suivante.
    bool rebindFailed{false};
};

/// Dessine l'interface pour l'image courante.
///
/// Déclarée sans le moindre type ImGui, à dessein : app/ ne possède que la
/// boucle de rendu, le point d'entrée n'a donc jamais besoin de la
/// bibliothèque d'interface dans son chemin d'inclusion.
void drawMainPanel(AppController& controller, PanelState& state);

} // namespace deuca::ui
