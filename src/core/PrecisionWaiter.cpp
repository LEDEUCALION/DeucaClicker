#include "core/PrecisionWaiter.hpp"

#include <algorithm>

#if defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#else
#include <thread>
#endif

namespace deuca
{
namespace
{

/// Signale au processeur que la boucle en cours est une attente active.
///
/// Ce n'est pas un appel système : sur x86 c'est une instruction qui réduit la
/// consommation et libère les ressources partagées du cœur au profit de l'autre
/// fil matériel. Rester dans la bibliothèque standard imposerait yield(), qui
/// passe par l'ordonnanceur et coûte bien plus que la marge qu'on essaie de
/// tenir.
inline void cpuRelax() noexcept
{
#if defined(_M_X64) || defined(_M_IX86)
    _mm_pause();
#else
    std::this_thread::yield();
#endif
}

} // namespace

Duration blockingPortion(Duration remaining, Duration spinBudget) noexcept
{
    if (remaining <= spinBudget)
    {
        return Duration::zero();
    }

    return remaining - spinBudget;
}

Duration effectiveSpinBudget(Duration requested, Duration granularity) noexcept
{
    return std::max(requested, granularity);
}

PrecisionWaiter::PrecisionWaiter(IBlockingSleeper& sleeper, Policy policy) noexcept
    : m_sleeper{&sleeper}, m_spinBudget{effectiveSpinBudget(policy.spinBudget, sleeper.granularity())}
{
}

Timestamp PrecisionWaiter::waitUntil(Timestamp deadline)
{
    const Duration blocking = blockingPortion(deadline - Clock::now(), m_spinBudget);
    if (blocking > Duration::zero())
    {
        m_sleeper->sleepFor(blocking);
    }

    // Le dormeur rend la main quelque part avant l'échéance, à sa granularité
    // près. Le reste se joue ici, où l'on peut viser la microseconde.
    Timestamp now = Clock::now();
    while (now < deadline)
    {
        cpuRelax();
        now = Clock::now();
    }

    return now;
}

Duration PrecisionWaiter::spinBudget() const noexcept
{
    return m_spinBudget;
}

} // namespace deuca
