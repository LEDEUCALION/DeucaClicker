#pragma once

#include "core/Waiting.hpp"

namespace deuca
{

/// Ce qu'un sondage apprend sur la cible.
struct ProbeResult
{
    /// Temps mis par la cible pour traiter un message vide.
    ///
    /// C'est une mesure de l'engorgement de sa file, pas du coût de ce qu'on
    /// lui demande : le message ne fait rien, tout le temps mesuré est du
    /// temps d'attente.
    Duration latency{};

    /// True si la cible ne répond plus du tout.
    ///
    /// Distinct d'une latence élevée : une application figée ne redeviendra pas
    /// réactive en ralentissant, il faut s'arrêter.
    bool hung{false};

    /// False si aucune cible n'a pu être sondée — pas de fenêtre au premier
    /// plan, ou fenêtre disparue entre deux mesures.
    bool valid{false};
};

/// Source de mesure de la réactivité de la cible.
///
/// Abstraite pour que la politique de cadence soit vérifiable sans fenêtre :
/// un sondeur simulé fournit la suite de latences qu'on veut éprouver, y
/// compris celles qu'on ne saurait pas provoquer à la demande.
class ITargetProbe
{
public:
    virtual ~ITargetProbe() = default;

    ITargetProbe(const ITargetProbe&) = delete;
    ITargetProbe& operator=(const ITargetProbe&) = delete;
    ITargetProbe(ITargetProbe&&) = delete;
    ITargetProbe& operator=(ITargetProbe&&) = delete;

    /// Mesure la réactivité de la cible courante.
    ///
    /// Bloque au plus le temps imparti par l'implantation. À n'appeler que
    /// depuis un fil dédié : sur le fil de cadence, chaque mesure interromprait
    /// les clics, ce qui est exactement l'inverse du but poursuivi.
    [[nodiscard]] virtual ProbeResult probe() = 0;

protected:
    ITargetProbe() = default;
};

} // namespace deuca
