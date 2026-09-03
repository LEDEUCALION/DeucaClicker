#include "core/GovernorPolicy.hpp"

#include <algorithm>

namespace deuca
{

double nextScale(double currentScale, const ProbeResult& result, const GovernorConfig& config) noexcept
{
    const double floor = std::clamp(config.minimumScale, 0.0, 1.0);
    const double scale = std::clamp(currentScale, floor, 1.0);

    // Sondage impossible : on ne sait rien, donc on ne bouge pas. Accélérer
    // sur une absence d'information reviendrait à supposer que tout va bien
    // parce qu'on n'a pas regardé.
    if (!result.valid)
    {
        return scale;
    }

    // Application figée : ralentir n'y changera rien, elle ne traite plus
    // aucun message. On descend au plancher et on laisse l'appelant décider
    // s'il coupe.
    if (result.hung)
    {
        return floor;
    }

    if (result.latency > config.latencyThreshold)
    {
        return std::max(floor, scale * config.backoffFactor);
    }

    return std::min(1.0, scale + config.recoveryStep);
}

Duration scaledPeriod(Duration base, double scale) noexcept
{
    // Borne basse arbitraire mais non nulle : un facteur mal transmis donne
    // ainsi une cadence très lente, défaut visible et corrigible, plutôt qu'une
    // division par zéro.
    constexpr double kSmallestScale = 0.001;

    const double clamped = std::clamp(scale, kSmallestScale, 1.0);
    const auto seconds = std::chrono::duration<double>{base}.count() / clamped;

    return std::chrono::duration_cast<Duration>(std::chrono::duration<double>{seconds});
}

} // namespace deuca
