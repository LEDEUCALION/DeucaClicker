#pragma once

#include "core/Waiting.hpp"

#include <cstddef>
#include <span>

namespace deuca
{

/// Résumé statistique d'une série de durées.
struct Summary
{
    std::size_t count{};
    Duration min{};
    Duration p50{};
    Duration p90{};
    Duration p99{};
    Duration max{};
    Duration mean{};
};

/// Percentile d'un échantillon **déjà trié**, par rang le plus proche.
///
/// Il existe plusieurs conventions de percentile et elles ne donnent pas le
/// même chiffre. Sur un projet dont l'argument est de publier des mesures,
/// taire laquelle est utilisée est le meilleur moyen de ne pas être cru : le
/// rang le plus proche, sans interpolation, index = ceil(fraction × n) − 1.
///
/// @param sorted    échantillon trié par ordre croissant.
/// @param fraction  entre 0 et 1, bornée si elle sort de l'intervalle.
/// @return zéro si l'échantillon est vide.
[[nodiscard]] Duration percentile(std::span<const Duration> sorted, double fraction) noexcept;

/// Trie une copie de l'échantillon et en tire le résumé.
///
/// Un échantillon vide renvoie un résumé à zéro plutôt que de lever : un banc
/// qui n'a rien pu mesurer doit le dire dans son tableau, pas interrompre la
/// campagne en cours.
[[nodiscard]] Summary summarize(std::span<const Duration> samples);

} // namespace deuca
