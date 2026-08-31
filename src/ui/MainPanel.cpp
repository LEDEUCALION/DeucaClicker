#include "ui/MainPanel.hpp"

#include "core/Version.hpp"

#include <imgui.h>

namespace deuca::ui
{

void drawMainPanel()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("DeucaClicker", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextUnformatted(deuca::buildBanner().c_str());
    ImGui::Separator();
    ImGui::TextDisabled("Shell only. The engine lands in the next branches.");

    ImGui::End();
}

} // namespace deuca::ui
