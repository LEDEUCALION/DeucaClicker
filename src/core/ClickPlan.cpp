#include "core/ClickPlan.hpp"

#include <algorithm>
#include <cmath>

namespace deuca
{

std::size_t pressesPerActivation(ClickStyle style) noexcept
{
    return style == ClickStyle::Double ? 2 : 1;
}

std::size_t eventsPerBurst(const ClickPlan& plan) noexcept
{
    return std::max<std::size_t>(1, plan.burstSize) * pressesPerActivation(plan.style) * 2;
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

double clicksPerSecondFromInterval(Duration interval) noexcept
{
    if (interval <= Duration::zero())
    {
        return 0.0;
    }

    const auto seconds = std::chrono::duration<double>{interval}.count();
    return 1.0 / seconds;
}

Duration intervalFromClicksPerSecond(double clicksPerSecond) noexcept
{
    if (!(clicksPerSecond > 0.0))
    {
        return Duration::zero();
    }

    return std::chrono::duration_cast<Duration>(std::chrono::duration<double>{1.0 / clicksPerSecond});
}

} // namespace deuca
