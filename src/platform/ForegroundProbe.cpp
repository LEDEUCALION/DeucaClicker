#include "platform/ForegroundProbe.hpp"

#include "platform/WindowsLean.hpp"

#include <algorithm>

namespace deuca::platform
{

ForegroundProbe::ForegroundProbe(Duration timeout) noexcept : m_timeout{timeout} {}

ProbeResult ForegroundProbe::probe()
{
    const HWND target = ::GetForegroundWindow();
    if (target == nullptr)
    {
        return ProbeResult{};
    }

    // Ne pas se sonder soi-même : notre propre file est toujours saine, et la
    // mesurer donnerait un asservissement qui ne freine jamais.
    DWORD targetProcess = 0;
    ::GetWindowThreadProcessId(target, &targetProcess);
    if (targetProcess == ::GetCurrentProcessId())
    {
        return ProbeResult{};
    }

    if (::IsHungAppWindow(target) != FALSE)
    {
        return ProbeResult{.latency = m_timeout, .hung = true, .valid = true};
    }

    const auto timeoutMs =
        static_cast<UINT>(std::chrono::duration_cast<std::chrono::milliseconds>(m_timeout).count());

    const Timestamp before = Clock::now();

    // WM_NULL ne fait rien, par définition. Tout le temps mesuré est donc du
    // temps d'attente dans la file, jamais du temps de traitement.
    //
    // ABORTIFHUNG rend la main immédiatement si le système sait déjà que la
    // fenêtre est bloquée, plutôt que d'attendre le délai entier.
    DWORD_PTR unused = 0;
    const LRESULT answered =
        ::SendMessageTimeoutW(target, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, timeoutMs, &unused);

    const Duration elapsed = Clock::now() - before;

    if (answered == 0)
    {
        // Zéro couvre deux cas : délai dépassé, ou fenêtre disparue entre son
        // obtention et l'envoi. Le second n'est pas un engorgement, et le code
        // d'erreur les sépare.
        const bool timedOut = ::GetLastError() == ERROR_TIMEOUT;
        return ProbeResult{.latency = elapsed, .hung = timedOut, .valid = timedOut};
    }

    return ProbeResult{.latency = elapsed, .hung = false, .valid = true};
}

} // namespace deuca::platform
