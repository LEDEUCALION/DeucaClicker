#include "platform/HighResolutionTimer.hpp"

#include "platform/WindowsLean.hpp"

namespace deuca::platform
{
namespace
{

/// Granularité d'un timer attendable créé avec le drapeau haute résolution.
constexpr Duration kHighResolutionGranularity = std::chrono::microseconds{500};

/// Sans ce drapeau, l'attente retombe sur le tick système : 64 Hz, soit
/// 15,625 ms. C'est la valeur que tout le monde cite comme « la précision de
/// Sleep ».
constexpr Duration kSystemTickGranularity = std::chrono::microseconds{15625};

/// SetWaitableTimer compte en unités de 100 nanosecondes.
constexpr Duration::rep kNanosecondsPerTick = 100;

} // namespace

HighResolutionTimer::HighResolutionTimer() noexcept
{
    // Réarmement automatique : un timer manuel resterait signalé après son
    // échéance, et l'attente suivante rendrait la main immédiatement.
    m_timer =
        ::CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (m_timer != nullptr)
    {
        m_highResolution = true;
        return;
    }

    // Le drapeau n'existe qu'à partir de Windows 10 1803. En dessous, un timer
    // ordinaire fait l'affaire : il est plus grossier, mais l'appelant lit
    // granularity() et ajuste sa marge en conséquence.
    m_timer = ::CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
}

HighResolutionTimer::~HighResolutionTimer()
{
    if (m_timer != nullptr)
    {
        ::CloseHandle(static_cast<HANDLE>(m_timer));
    }
}

Duration HighResolutionTimer::granularity() const noexcept
{
    return m_highResolution ? kHighResolutionGranularity : kSystemTickGranularity;
}

void HighResolutionTimer::sleepFor(Duration duration)
{
    if (m_timer == nullptr || duration <= Duration::zero())
    {
        return;
    }

    LARGE_INTEGER dueTime{};
    // Valeur négative : délai relatif plutôt qu'instant absolu. La troncature
    // de la division fait dormir légèrement moins que demandé, ce qui est le
    // bon sens de l'erreur — l'attente active qui suit rattrape un réveil
    // précoce, alors qu'un réveil tardif est une échéance manquée.
    dueTime.QuadPart = -(duration.count() / kNanosecondsPerTick);
    if (dueTime.QuadPart == 0)
    {
        return;
    }

    if (::SetWaitableTimer(static_cast<HANDLE>(m_timer), &dueTime, 0, nullptr, nullptr, FALSE) != FALSE)
    {
        ::WaitForSingleObject(static_cast<HANDLE>(m_timer), INFINITE);
    }
}

bool HighResolutionTimer::isHighResolution() const noexcept
{
    return m_highResolution;
}

bool HighResolutionTimer::isValid() const noexcept
{
    return m_timer != nullptr;
}

} // namespace deuca::platform
