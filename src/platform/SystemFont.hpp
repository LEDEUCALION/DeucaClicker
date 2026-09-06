#pragma once

#include <string>

namespace deuca::platform
{

/// Poids d'une police système.
enum class FontWeight
{
    Regular,
    SemiBold,
    Bold,
};

/// Chemin complet vers un fichier de police du système.
///
/// Charger la police livrée avec Windows plutôt que d'en embarquer une : le
/// rendu paraît natif, le binaire ne grossit pas, et aucune licence
/// supplémentaire n'entre dans le projet. Le prix est qu'il faut prévoir son
/// absence — une installation allégée ou un futur Windows peut l'avoir
/// déplacée.
///
/// @return une chaîne vide si le fichier n'existe pas. L'appelant doit alors se
///         rabattre sur la police intégrée, dégradée mais toujours lisible.
[[nodiscard]] std::string segoeUiPath(FontWeight weight);

} // namespace deuca::platform
