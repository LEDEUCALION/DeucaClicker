#include "ui/MainPanel.hpp"

#include "core/Version.hpp"
#include "ui/AppController.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <climits>
#include <cstdint>
#include <string>

namespace deuca::ui
{
namespace
{

/// Touches proposées pour le raccourci.
///
/// Limité aux touches de fonction : ce sont les seules qui ne servent presque
/// jamais dans une application ou un jeu, donc les seules qu'on puisse
/// réquisitionner sans gêner l'utilisateur. Le service en accepte d'autres ;
/// c'est l'interface qui se restreint.
constexpr std::array kFunctionKeyNames{"F1", "F2", "F3", "F4",  "F5",  "F6",
                                       "F7", "F8", "F9", "F10", "F11", "F12"};

/// VK_F1 vaut 0x70 et les suivantes se succèdent.
constexpr std::uint32_t kFirstFunctionKey = 0x70;

constexpr ImVec4 kRunningColour{0.35f, 0.85f, 0.45f, 1.0f};
constexpr ImVec4 kIdleColour{0.65f, 0.68f, 0.72f, 1.0f};
constexpr ImVec4 kWarningColour{0.95f, 0.62f, 0.25f, 1.0f};

[[nodiscard]] const char* buttonName(MouseButton button) noexcept
{
    switch (button)
    {
    case MouseButton::Right:
        return "Droit";
    case MouseButton::Middle:
        return "Milieu";
    case MouseButton::Left:
        break;
    }

    return "Gauche";
}

[[nodiscard]] std::string describeHotkey(const platform::Hotkey& hotkey)
{
    std::string text;

    if (platform::contains(hotkey.modifiers, platform::Modifier::Control))
    {
        text += "Ctrl + ";
    }
    if (platform::contains(hotkey.modifiers, platform::Modifier::Alt))
    {
        text += "Alt + ";
    }
    if (platform::contains(hotkey.modifiers, platform::Modifier::Shift))
    {
        text += "Maj + ";
    }

    const auto index = static_cast<std::size_t>(hotkey.virtualKey - kFirstFunctionKey);
    if (index < kFunctionKeyNames.size())
    {
        text += kFunctionKeyNames[index];
    }
    else
    {
        text += "touche inconnue";
    }

    return text;
}

void drawHeader(AppController& controller)
{
    const bool running = controller.isRunning();
    const std::string hotkey = describeHotkey(controller.panicHotkey());

    ImGui::TextColored(running ? kRunningColour : kIdleColour, running ? "En marche" : "A l'arret");
    ImGui::SameLine();
    ImGui::TextDisabled("|  %s pour demarrer ou arreter", hotkey.c_str());

    if (!controller.panicHotkeyActive())
    {
        ImGui::TextColored(kWarningColour,
                           "Raccourci d'arret indisponible : une autre application le detient.");
        ImGui::TextColored(kWarningColour, "Choisissez-en un autre dans Reglages avant de lancer.");
    }

    ImGui::Spacing();

    // Le démarrage est refusé sans arrêt d'urgence, et le bouton le montre
    // plutôt que d'échouer en silence une fois pressé.
    ImGui::BeginDisabled(!controller.panicHotkeyActive());

    const ImVec2 size{-FLT_MIN, 34.0f};
    if (ImGui::Button(running ? "Arreter" : "Demarrer", size))
    {
        controller.toggle();
    }

    ImGui::EndDisabled();

    const std::string refusal = controller.refusalMessage();
    if (!refusal.empty() && !running)
    {
        ImGui::TextColored(kWarningColour, "%s", refusal.c_str());
    }
}

void drawClickingSection(AppController& controller)
{
    if (!ImGui::CollapsingHeader("Clic", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ClickPlan& plan = controller.plan();

    if (ImGui::BeginCombo("Bouton", buttonName(plan.button)))
    {
        for (const MouseButton candidate : {MouseButton::Left, MouseButton::Right, MouseButton::Middle})
        {
            const bool selected = plan.button == candidate;
            if (ImGui::Selectable(buttonName(candidate), selected))
            {
                plan.button = candidate;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const bool isDouble = plan.style == ClickStyle::Double;
    if (ImGui::BeginCombo("Type", isDouble ? "Double clic" : "Clic simple"))
    {
        if (ImGui::Selectable("Clic simple", !isDouble))
        {
            plan.style = ClickStyle::Single;
        }
        if (ImGui::Selectable("Double clic", isDouble))
        {
            plan.style = ClickStyle::Double;
        }
        ImGui::EndCombo();
    }

    ImGui::SeparatorText("Intervalle entre clics");

    IntervalFields& interval = controller.interval();
    bool changed = false;

    ImGui::PushItemWidth(70.0f);
    changed = ImGui::InputInt("h", &interval.hours, 0) || changed;
    ImGui::SameLine();
    changed = ImGui::InputInt("min", &interval.minutes, 0) || changed;
    ImGui::SameLine();
    changed = ImGui::InputInt("s", &interval.seconds, 0) || changed;
    ImGui::SameLine();
    changed = ImGui::InputInt("ms", &interval.milliseconds, 0) || changed;
    ImGui::PopItemWidth();

    if (changed)
    {
        // Des valeurs négatives donneraient un intervalle négatif, donc une
        // cadence nulle, donc un refus de démarrer sans explication visible.
        interval.hours = std::max(0, interval.hours);
        interval.minutes = std::max(0, interval.minutes);
        interval.seconds = std::max(0, interval.seconds);
        interval.milliseconds = std::max(0, interval.milliseconds);
        controller.applyInterval();
    }

    ImGui::Text("Soit %.1f %s par seconde", plan.clicksPerSecond,
                plan.style == ClickStyle::Double ? "doubles-clics" : "clics");

    if (plan.style == ClickStyle::Double)
    {
        ImGui::TextDisabled("Soit %.1f clics physiques par seconde.", plan.clicksPerSecond * 2.0);
    }

    const auto effectiveMs =
        std::chrono::duration<double, std::milli>{controller.effectiveInterval()}.count();
    ImGui::TextDisabled("Applique : %.2f ms entre deux clics", effectiveMs);

    ImGui::SeparatorText("Groupement");

    int burst = static_cast<int>(plan.burstSize);
    if (ImGui::SliderInt("Clics par lot", &burst, 1, 128))
    {
        plan.burstSize = static_cast<std::size_t>(std::max(1, burst));
    }

    ImGui::TextDisabled("Un lot est insere d'un bloc, sans qu'un mouvement reel");
    ImGui::TextDisabled("puisse s'y glisser. C'est le levier de debit.");

    ImGui::SeparatorText("Repetition");

    bool limited = plan.repeatLimit > 0;
    if (ImGui::RadioButton("Jusqu'a l'arret", !limited))
    {
        plan.repeatLimit = 0;
        limited = false;
    }

    if (ImGui::RadioButton("Un nombre de fois", limited))
    {
        // Une valeur de depart plutot que zero : passer sur ce mode avec une
        // limite nulle ferait un moteur qui refuse de demarrer sans raison
        // visible.
        plan.repeatLimit = plan.repeatLimit > 0 ? plan.repeatLimit : 100;
        limited = true;
    }

    ImGui::BeginDisabled(!limited);
    int repeats = static_cast<int>(std::min<std::uint64_t>(plan.repeatLimit, INT_MAX));
    ImGui::PushItemWidth(120.0f);
    if (ImGui::InputInt("Repetitions", &repeats, 1, 100) && limited)
    {
        plan.repeatLimit = static_cast<std::uint64_t>(std::max(1, repeats));
    }
    ImGui::PopItemWidth();
    ImGui::EndDisabled();
}

void drawTargetsSection(AppController& controller)
{
    if (!ImGui::CollapsingHeader("Cibles"))
    {
        return;
    }

    const ClickPlan& plan = controller.plan();

    if (plan.targets.empty())
    {
        ImGui::TextDisabled("Aucune cible : les clics partent la ou se trouve");
        ImGui::TextDisabled("le curseur, sans le deplacer.");
    }
    else
    {
        for (std::size_t i = 0; i < plan.targets.size(); ++i)
        {
            ImGui::Text("%zu.  x = %d,  y = %d", i + 1, plan.targets[i].x, plan.targets[i].y);
        }
        ImGui::TextDisabled("Les cibles sont visitees en boucle, une par clic.");
    }

    ImGui::Spacing();

    if (ImGui::Button("Capturer la position du curseur"))
    {
        controller.captureTarget();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(plan.targets.empty());
    if (ImGui::Button("Vider"))
    {
        controller.clearTargets();
    }
    ImGui::EndDisabled();
}

void drawTelemetrySection(AppController& controller)
{
    if (!ImGui::CollapsingHeader("Telemetrie", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    const EngineSnapshot snapshot = controller.snapshot();

    ImGui::Text("Lots soumis : %llu", static_cast<unsigned long long>(snapshot.burstsSubmitted));
    ImGui::Text("Clics emis  : %llu", static_cast<unsigned long long>(snapshot.clicksEmitted));

    if (snapshot.eventsRejected > 0)
    {
        ImGui::TextColored(kWarningColour, "Evenements refuses : %llu",
                           static_cast<unsigned long long>(snapshot.eventsRejected));
    }

    if (snapshot.blockedSubmissions > 0)
    {
        ImGui::TextColored(kWarningColour, "Lots bloques : %llu",
                           static_cast<unsigned long long>(snapshot.blockedSubmissions));
        ImGui::TextDisabled("L'entree est bloquee par un autre processus.");
    }

    ImGui::SeparatorText("Reactivite de la cible");

    bool governor = controller.governorEnabled();
    if (ImGui::Checkbox("Adapter la cadence a la cible", &governor))
    {
        controller.setGovernorEnabled(governor);
    }

    if (!governor)
    {
        ImGui::TextDisabled("Desactive : la cadence demandee est appliquee telle");
        ImGui::TextDisabled("quelle, meme si la cible ne suit pas.");
        return;
    }

    const double scale = controller.governorScale();
    const auto latencyMs = std::chrono::duration<double, std::milli>{controller.governorLatency()}.count();

    ImGui::Text("Latence de la cible : %.1f ms", latencyMs);

    const ImVec4 scaleColour = scale < 0.99 ? kWarningColour : kRunningColour;
    ImGui::TextColored(scaleColour, "Cadence appliquee : %.0f %% du reglage", scale * 100.0);

    if (controller.targetHung())
    {
        ImGui::TextColored(kWarningColour, "La cible ne repond plus.");
        ImGui::TextDisabled("Ralentir n'y changera rien : elle ne traite plus");
        ImGui::TextDisabled("aucun message. Mieux vaut arreter.");
    }
    else if (scale < 0.99)
    {
        ImGui::TextDisabled("La file de la cible s'engorge : la cadence recule");
        ImGui::TextDisabled("et remontera d'elle-meme des qu'elle suivra.");
    }
}

void drawSettingsSection(AppController& controller, PanelState& state)
{
    if (!ImGui::CollapsingHeader("Reglages"))
    {
        return;
    }

    ImGui::SeparatorText("Raccourci de demarrage et d'arret");

    ImGui::Checkbox("Ctrl", &state.pendingControl);
    ImGui::SameLine();
    ImGui::Checkbox("Alt", &state.pendingAlt);
    ImGui::SameLine();
    ImGui::Checkbox("Maj", &state.pendingShift);

    ImGui::PushItemWidth(90.0f);
    ImGui::Combo("Touche", &state.pendingFunctionKeyIndex, kFunctionKeyNames.data(),
                 static_cast<int>(kFunctionKeyNames.size()));
    ImGui::PopItemWidth();

    if (ImGui::Button("Appliquer le raccourci"))
    {
        platform::Modifier modifiers = platform::Modifier::None;
        if (state.pendingControl)
        {
            modifiers = modifiers | platform::Modifier::Control;
        }
        if (state.pendingAlt)
        {
            modifiers = modifiers | platform::Modifier::Alt;
        }
        if (state.pendingShift)
        {
            modifiers = modifiers | platform::Modifier::Shift;
        }

        const platform::Hotkey candidate{
            .modifiers = modifiers,
            .virtualKey = kFirstFunctionKey + static_cast<std::uint32_t>(state.pendingFunctionKeyIndex)};

        state.rebindFailed = !controller.rebindPanicHotkey(candidate);
    }

    if (state.rebindFailed)
    {
        ImGui::TextColored(kWarningColour, "Combinaison refusee : une autre application la detient.");
        ImGui::TextColored(kWarningColour, "L'ancienne reste active.");
    }
}

} // namespace

void drawMainPanel(AppController& controller, PanelState& state)
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("DeucaClicker", nullptr, ImGuiWindowFlags_NoCollapse);

    drawHeader(controller);
    ImGui::Spacing();

    drawClickingSection(controller);
    drawTargetsSection(controller);
    drawTelemetrySection(controller);
    drawSettingsSection(controller, state);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("%s", deuca::buildBanner().c_str());

    ImGui::End();
}

} // namespace deuca::ui
