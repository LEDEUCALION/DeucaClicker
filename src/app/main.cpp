#include "platform/WindowsLean.hpp"

#include "platform/ProcessSetup.hpp"
#include "ui/AppController.hpp"
#include "ui/ImGuiHost.hpp"
#include "ui/MainPanel.hpp"

namespace
{

/// Durée d'attente pendant que la fenêtre est réduite, en millisecondes.
///
/// La cadence de clic ne dépend pas de cette boucle : elle tourne sur son
/// propre fil, à sa propre priorité. Ralentir l'affichage n'a donc aucun effet
/// sur ce que fait le moteur.
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

    // Construit après la fenêtre : le service de raccourcis démarre son fil dès
    // sa construction, et rien ne doit pouvoir déclencher un clic avant que
    // l'interface soit là pour le montrer.
    deuca::ui::AppController controller;
    deuca::ui::PanelState panelState;

    while (host.pumpMessages())
    {
        if (!host.beginFrame())
        {
            ::Sleep(kIdleSleepMs);
            continue;
        }

        deuca::ui::drawMainPanel(controller, panelState);
        host.endFrame();
    }

    return 0;
}
