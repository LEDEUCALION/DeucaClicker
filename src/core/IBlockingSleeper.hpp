#pragma once

#include "core/Waiting.hpp"

namespace deuca
{

/// Attente bloquante fournie par la plateforme.
///
/// Abstraite pour une seule raison, mais elle est décisive : la politique
/// d'attente de PrecisionWaiter devient vérifiable sans primitive système ni
/// horloge réelle. Un dormeur simulé qui se contente d'enregistrer ce qu'on
/// lui demande suffit à prouver l'arithmétique de cadence, qui est justement
/// la partie qui casse en silence.
class IBlockingSleeper
{
public:
    virtual ~IBlockingSleeper() = default;

    IBlockingSleeper(const IBlockingSleeper&) = delete;
    IBlockingSleeper& operator=(const IBlockingSleeper&) = delete;
    IBlockingSleeper(IBlockingSleeper&&) = delete;
    IBlockingSleeper& operator=(IBlockingSleeper&&) = delete;

    /// Granularité en dessous de laquelle demander une attente n'a plus de
    /// sens : le dormeur rendra la main plus tard que l'échéance demandée.
    ///
    /// C'est cette valeur qui borne par le bas la marge d'attente active :
    /// bloquer jusqu'à « échéance moins 300 µs » avec un dormeur à 15 ms de
    /// granularité, c'est manquer l'échéance de 15 ms une fois sur deux.
    [[nodiscard]] virtual Duration granularity() const noexcept = 0;

    /// Bloque le fil d'exécution appelant pendant au moins la durée demandée.
    /// Une durée nulle ou négative doit rendre la main immédiatement.
    virtual void sleepFor(Duration duration) = 0;

protected:
    IBlockingSleeper() = default;
};

} // namespace deuca
