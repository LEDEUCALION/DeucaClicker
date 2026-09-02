#include "core/Statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace deuca
{

Duration percentile(std::span<const Duration> sorted, double fraction) noexcept
{
    if (sorted.empty())
    {
        return Duration::zero();
    }

    const double clamped = std::clamp(fraction, 0.0, 1.0);
    const auto size = static_cast<std::ptrdiff_t>(sorted.size());

    // Rang le plus proche : on prend l'échantillon réellement observé plutôt
    // que d'interpoler entre deux voisins. Sur une distribution de latences,
    // interpoler invente une valeur que la machine n'a jamais produite.
    auto rank = static_cast<std::ptrdiff_t>(std::ceil(clamped * static_cast<double>(size))) - 1;
    rank = std::clamp<std::ptrdiff_t>(rank, 0, size - 1);

    return sorted[static_cast<std::size_t>(rank)];
}

Summary summarize(std::span<const Duration> samples)
{
    if (samples.empty())
    {
        return Summary{};
    }

    // Copie plutôt que tri en place : l'appelant garde son échantillon dans
    // l'ordre d'acquisition, qui est le seul ordre où l'on peut encore voir
    // une dérive au fil du temps.
    std::vector<Duration> sorted{samples.begin(), samples.end()};
    std::ranges::sort(sorted);

    const auto total = std::accumulate(sorted.begin(), sorted.end(), Duration::zero());

    return Summary{
        .count = sorted.size(),
        .min = sorted.front(),
        .p50 = percentile(sorted, 0.50),
        .p90 = percentile(sorted, 0.90),
        .p99 = percentile(sorted, 0.99),
        .max = sorted.back(),
        .mean = total / static_cast<Duration::rep>(sorted.size()),
    };
}

} // namespace deuca
