#pragma once

#include "core/IBlockingSleeper.hpp"
#include "core/Waiting.hpp"

namespace deuca
{

/// Part de l'attente qu'il faut confier au dormeur bloquant.
///
/// Fonction libre et sans état à dessein : c'est l'arithmétique qui décide de
/// la cadence, et elle se vérifie sans horloge ni système d'exploitation.
///
/// @param remaining   temps restant jusqu'à l'échéance ; peut être négatif.
/// @param spinBudget  marge finale réservée à l'attente active.
/// @return la durée à passer bloqué, jamais négative.
[[nodiscard]] Duration blockingPortion(Duration remaining, Duration spinBudget) noexcept;

/// Marge d'attente active réellement applicable.
///
/// La marge demandée ne peut pas descendre sous la granularité du dormeur :
/// en dessous, le réveil arriverait après l'échéance et il ne resterait plus
/// rien à rattraper en attente active.
[[nodiscard]] Duration effectiveSpinBudget(Duration requested, Duration granularity) noexcept;

/// Attend une échéance avec une précision meilleure que celle du dormeur seul.
///
/// L'escalier a deux marches. On bloque tant que l'échéance est loin, ce qui
/// ne coûte rien au processeur, puis on termine en attente active sur les
/// dernières microsecondes, là où aucune primitive bloquante n'est assez fine.
///
/// La marge d'attente active est un réglage, pas une constante : c'est un
/// arbitrage entre précision et consommation, et il n'a pas la même réponse
/// sur un portable et sur une tour.
class PrecisionWaiter
{
public:
    struct Policy
    {
        /// Marge finale traitée en attente active.
        Duration spinBudget{std::chrono::microseconds{300}};
    };

    PrecisionWaiter(IBlockingSleeper& sleeper, Policy policy = {}) noexcept;

    /// Attend jusqu'à l'échéance et renvoie l'instant réel du réveil.
    ///
    /// Renvoie immédiatement si l'échéance est déjà passée : rattraper un
    /// retard en dormant davantage ne ferait que l'aggraver.
    Timestamp waitUntil(Timestamp deadline);

    /// Marge d'attente active après recalage sur la granularité du dormeur.
    [[nodiscard]] Duration spinBudget() const noexcept;

private:
    IBlockingSleeper* m_sleeper;
    Duration m_spinBudget;
};

} // namespace deuca
