#pragma once

namespace deuca::ui
{

/// Dessine les panneaux de l'application pour l'image courante.
///
/// Déclarée sans le moindre type ImGui, à dessein : app/ ne possède que la
/// boucle de rendu, le point d'entrée n'a donc jamais besoin de la
/// bibliothèque d'interface dans son chemin d'inclusion.
void drawMainPanel();

} // namespace deuca::ui
