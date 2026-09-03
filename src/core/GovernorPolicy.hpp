#pragma once

#include "core/ITargetProbe.hpp"
#include "core/Waiting.hpp"

namespace deuca
{

/// Réglages de la boucle d'asservissement.
struct GovernorConfig
{
    /// Latence au-delà de laquelle on considère la cible en difficulté.
    ///
    /// Une file saine répond à un message vide en bien moins d'une
    /// milliseconde. Le seuil est volontairement plus haut : on ne veut pas
    /// réagir au bruit d'ordonnancement, seulement à un engorgement réel.
    Duration latencyThreshold{std::chrono::milliseconds{20}};

    /// Facteur appliqué quand la cible peine.
    ///
    /// Décroissance multiplicative, comme le contrôle de congestion des
    /// réseaux et pour la même raison : quand on ignore où est la limite, il
    /// faut reculer vite. Reculer doucement laisserait la file s'engorger
    /// pendant qu'on tergiverse.
    double backoffFactor{0.5};

    /// Part de la cadence demandée regagnée à chaque mesure saine.
    ///
    /// Croissance additive : on remonte par petits pas, sans jamais retenter
    /// d'un coup la cadence qui vient d'échouer.
    double recoveryStep{0.05};

    /// Plancher, en fraction de la cadence demandée.
    ///
    /// Empêche l'asservissement de réduire l'outil au silence : en dessous, le
    /// bon diagnostic n'est plus « ralentis » mais « cette cible ne suit pas ».
    double minimumScale{0.02};
};

/// Facteur d'échelle appliqué à la cadence demandée, entre le plancher et un.
///
/// Exprimé en proportion plutôt qu'en clics par seconde : la cadence demandée
/// peut changer pendant qu'une session tourne, et un facteur reste valable là
/// où une valeur absolue deviendrait absurde.
[[nodiscard]] double nextScale(double currentScale, const ProbeResult& result,
                               const GovernorConfig& config) noexcept;

/// Applique un facteur d'échelle à une période.
///
/// Un facteur plus petit allonge la période : ralentir, c'est attendre plus
/// longtemps entre deux lots. Le facteur est borné par le bas pour qu'un zéro
/// mal transmis n'aboutisse pas à une division par zéro, mais à une cadence
/// très lente — un défaut visible plutôt qu'un plantage.
[[nodiscard]] Duration scaledPeriod(Duration base, double scale) noexcept;

} // namespace deuca
