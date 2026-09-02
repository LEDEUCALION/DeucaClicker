#include "core/ClickPlan.hpp"

#include <algorithm>
#include <cmath>

namespace deuca
{

std::size_t eventsPerBurst(const ClickPlan& plan) noexcept
{
    return std::max<std::size_t>(1, plan.burstSize) * 2;
}

Duration burstPeriod(const ClickPlan& plan) noexcept
{
    if (!(plan.clicksPerSecond > 0.0))
    {
        return Duration::zero();
    }

    const double clicks = static_cast<double>(std::max<std::size_t>(1, plan.burstSize));
    const double seconds = clicks / plan.clicksPerSecond;

    return std::chrono::duration_cast<Duration>(std::chrono::duration<double>{seconds});
}

} // namespace deuca
