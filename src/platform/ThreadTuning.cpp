#include "platform/ThreadTuning.hpp"

#include "platform/WindowsLean.hpp"

#include <avrt.h>

namespace deuca::platform
{
namespace
{

/// Nom de tâche MMCSS. « Pro Audio » est la classe la plus prioritaire des
/// classes multimédia par défaut, et c'est celle que visent les moteurs audio
/// à faible latence — le besoin le plus proche du nôtre.
constexpr wchar_t kMmcssTaskName[] = L"Pro Audio";

[[nodiscard]] int toWin32Priority(ThreadPriority priority) noexcept
{
    switch (priority)
    {
    case ThreadPriority::AboveNormal:
        return THREAD_PRIORITY_ABOVE_NORMAL;
    case ThreadPriority::Highest:
        return THREAD_PRIORITY_HIGHEST;
    case ThreadPriority::TimeCritical:
        return THREAD_PRIORITY_TIME_CRITICAL;
    case ThreadPriority::Normal:
        break;
    }

    return THREAD_PRIORITY_NORMAL;
}

[[nodiscard]] ThreadPriority fromWin32Priority(int priority) noexcept
{
    switch (priority)
    {
    case THREAD_PRIORITY_ABOVE_NORMAL:
        return ThreadPriority::AboveNormal;
    case THREAD_PRIORITY_HIGHEST:
        return ThreadPriority::Highest;
    case THREAD_PRIORITY_TIME_CRITICAL:
        return ThreadPriority::TimeCritical;
    default:
        break;
    }

    return ThreadPriority::Normal;
}

} // namespace

ThreadPriority currentThreadPriority() noexcept
{
    const int priority = ::GetThreadPriority(::GetCurrentThread());
    if (priority == THREAD_PRIORITY_ERROR_RETURN)
    {
        return ThreadPriority::Normal;
    }

    return fromWin32Priority(priority);
}

ScopedThreadPriority::ScopedThreadPriority(ThreadPriority priority) noexcept
    : m_previous{currentThreadPriority()}
{
    m_active = ::SetThreadPriority(::GetCurrentThread(), toWin32Priority(priority)) != FALSE;
}

ScopedThreadPriority::~ScopedThreadPriority()
{
    if (m_active)
    {
        ::SetThreadPriority(::GetCurrentThread(), toWin32Priority(m_previous));
    }
}

bool ScopedThreadPriority::isActive() const noexcept
{
    return m_active;
}

ScopedMmcssTask::ScopedMmcssTask() noexcept
{
    // L'index de tâche est renseigné par l'appel et doit valoir zéro en entrée.
    DWORD taskIndex = 0;
    m_task = ::AvSetMmThreadCharacteristicsW(kMmcssTaskName, &taskIndex);
}

ScopedMmcssTask::~ScopedMmcssTask()
{
    if (m_task != nullptr)
    {
        ::AvRevertMmThreadCharacteristics(static_cast<HANDLE>(m_task));
    }
}

bool ScopedMmcssTask::isActive() const noexcept
{
    return m_task != nullptr;
}

ScopedPowerThrottlingOptOut::ScopedPowerThrottlingOptOut() noexcept
{
    PROCESS_POWER_THROTTLING_STATE state{};
    state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;

    // ControlMask désigne les politiques que l'on prend en main, StateMask
    // celles que l'on active. Les prendre en main en laissant StateMask à zéro
    // revient à les refuser explicitement.
    state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    state.StateMask = 0;

#ifdef PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
    // Windows 11 seulement. Sans ce drapeau, la haute résolution de nos timers
    // est retirée dès que la fenêtre est réduite ou occultée.
    state.ControlMask |= PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
#endif

    m_active = ::SetProcessInformation(::GetCurrentProcess(), ProcessPowerThrottling, &state,
                                       sizeof(state)) != FALSE;
}

ScopedPowerThrottlingOptOut::~ScopedPowerThrottlingOptOut()
{
    if (!m_active)
    {
        return;
    }

    // Les deux masques à zéro rendent la décision au système, ce qui n'est pas
    // la même chose que forcer le bridage.
    PROCESS_POWER_THROTTLING_STATE state{};
    state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    state.ControlMask = 0;
    state.StateMask = 0;

    ::SetProcessInformation(::GetCurrentProcess(), ProcessPowerThrottling, &state, sizeof(state));
}

bool ScopedPowerThrottlingOptOut::isActive() const noexcept
{
    return m_active;
}

ScopedThreadTuning::ScopedThreadTuning(TuningOptions options)
{
    // L'ordre a son importance : on sort du bridage énergétique avant de
    // demander une priorité, sinon la priorité s'applique à un thread que
    // l'ordonnanceur s'apprête à parquer sur un cœur lent.
    if (options.disablePowerThrottling)
    {
        m_power = std::make_unique<ScopedPowerThrottlingOptOut>();
    }

    if (options.pinToPerformanceCores)
    {
        m_affinity = std::make_unique<ScopedPerformanceCoreAffinity>();
    }

    if (options.useMmcss)
    {
        m_mmcss = std::make_unique<ScopedMmcssTask>();
    }

    if (options.raisePriority)
    {
        m_priority = std::make_unique<ScopedThreadPriority>(options.priority);
    }
}

ScopedThreadTuning::~ScopedThreadTuning() = default;

bool ScopedThreadTuning::mmcssActive() const noexcept
{
    return m_mmcss != nullptr && m_mmcss->isActive();
}

bool ScopedThreadTuning::priorityActive() const noexcept
{
    return m_priority != nullptr && m_priority->isActive();
}

bool ScopedThreadTuning::affinityActive() const noexcept
{
    return m_affinity != nullptr && m_affinity->isActive();
}

bool ScopedThreadTuning::powerThrottlingDisabled() const noexcept
{
    return m_power != nullptr && m_power->isActive();
}

} // namespace deuca::platform
