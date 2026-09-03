#pragma once

#include "core/ClickEvent.hpp"
#include "core/Waiting.hpp"

#include <cstddef>
#include <vector>

namespace deuca
{

/// Ce qu'il faut cliquer, et à quel rythme.
///
/// Type valeur sans comportement : le plan décrit une intention, il ne
/// l'exécute pas. C'est ce qui permet à l'interface de le construire, aux
/// tests de le fabriquer de toutes pièces et au moteur de le consommer sans
/// qu'aucun des trois n'ait à connaître les deux autres.
struct ClickPlan
{
    MouseButton button{MouseButton::Left};

    /// Cadence visée, en clics par seconde.
    double clicksPerSecond{10.0};

    /// Nombre de clics assemblés dans un même lot soumis au flux d'entrée.
    ///
    /// C'est le levier de débit. Un lot est inséré d'un bloc et n'est pas
    /// entrelacé avec les autres entrées, l'utilisateur compris : au-delà de
    /// quelques centaines de clics par seconde, grouper est la seule façon de
    /// ne pas payer un appel système par clic.
    std::size_t burstSize{1};

    /// Points à viser, dans l'ordre, en boucle.
    ///
    /// Liste vide : on clique là où se trouve le curseur, sans le déplacer.
    std::vector<ScreenPoint> targets{};
};

/// Nombre d'événements produits par un lot : un appui et un relâchement par
/// clic.
[[nodiscard]] std::size_t eventsPerBurst(const ClickPlan& plan) noexcept;

/// Délai entre deux soumissions de lot.
///
/// Un lot de dix clics à mille clics par seconde se soumet toutes les dix
/// millisecondes, pas toutes les millisecondes.
///
/// @return zéro si la cadence est nulle ou négative, ce que l'appelant doit
///         traiter comme « plan inexploitable » plutôt que comme « aussi vite
///         que possible ».
[[nodiscard]] Duration burstPeriod(const ClickPlan& plan) noexcept;

/// Convertit un intervalle entre clics en cadence.
///
/// Les deux expressions du même réglage coexistent parce qu'aucune ne convient
/// à tout le monde : « toutes les 30 ms » se lit mieux pour une automatisation
/// lente, « 500 clics par seconde » pour une rafale. L'interface propose les
/// deux et se sert de ces conversions pour les tenir d'accord.
///
/// @return zéro si l'intervalle est nul ou négatif.
[[nodiscard]] double clicksPerSecondFromInterval(Duration interval) noexcept;

/// Convertit une cadence en intervalle entre clics.
///
/// @return zéro si la cadence est nulle ou négative.
[[nodiscard]] Duration intervalFromClicksPerSecond(double clicksPerSecond) noexcept;

} // namespace deuca
