#include "platform/WindowsLean.hpp"

#include "platform/ProcessSetup.hpp"
#include "ui/ImGuiHost.hpp"
#include "ui/MainPanel.hpp"

namespace
{

/// Durée d'attente pendant que la fenêtre est réduite, en millisecondes. Assez
/// longue pour ne pas consommer un cœur sur une fenêtre invisible, assez courte
/// pour que la restauration paraisse instantanée.
constexpr DWORD kIdleSleepMs = 16;

} // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Avant l'existence de toute fenêtre : Windows fige le contexte de prise en
    // charge du DPI à la première utilisation et ignore les changements ensuite.
    deuca::platform::enablePerMonitorDpiAwareness();

    deuca::ui::ImGuiHost host{deuca::ui::ImGuiHost::Config{}};
    if (!host.isValid())
    {
        ::MessageBoxW(nullptr, L"Could not create the window or the Direct3D device.", L"DeucaClicker",
                      MB_ICONERROR | MB_OK);
        return 1;
    }

    while (host.pumpMessages())
    {
        if (!host.beginFrame())
        {
            ::Sleep(kIdleSleepMs);
            continue;
        }

        deuca::ui::drawMainPanel();
        host.endFrame();
    }

    return 0;
}
