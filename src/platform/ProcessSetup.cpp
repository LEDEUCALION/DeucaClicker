#include "platform/ProcessSetup.hpp"

#include "platform/WindowsLean.hpp"

namespace deuca::platform
{

bool enablePerMonitorDpiAwareness() noexcept
{
    // V2 plutôt que V1 : c'est le seul contexte où les zones non clientes — la
    // barre de titre, les barres de défilement que Windows dessine pour nous —
    // suivent la mise à l'échelle écran par écran, comme la zone client.
    return ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE;
}

} // namespace deuca::platform
