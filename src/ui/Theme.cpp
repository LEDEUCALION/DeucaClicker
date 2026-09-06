#include "ui/Theme.hpp"

#include <algorithm>
#include <cmath>

namespace deuca::ui::theme
{
namespace
{

[[nodiscard]] ImVec4 toVec4(ImU32 packed) noexcept
{
    return ImGui::ColorConvertU32ToFloat4(packed);
}

/// Applique un multiplicateur à l'alpha d'une couleur empaquetée.
[[nodiscard]] ImU32 withAlphaScale(ImU32 packed, float factor) noexcept
{
    const auto alpha = static_cast<float>((packed >> IM_COL32_A_SHIFT) & 0xFF);
    const auto scaled = static_cast<ImU32>(std::clamp(alpha * factor, 0.0f, 255.0f));

    return (packed & ~static_cast<ImU32>(0xFFu << IM_COL32_A_SHIFT)) | (scaled << IM_COL32_A_SHIFT);
}

} // namespace

void drawShadow(ImDrawList& list, ImVec2 min, ImVec2 max, std::span<const ShadowLayer> layers,
                float intensity)
{
    // Les couches sont parcourues de la plus large à la plus serrée : le halo
    // diffus se pose d'abord, les cernes plus denses par-dessus. L'ordre
    // inverse donnerait un voile uniforme au lieu d'un dégradé.
    for (const ShadowLayer& layer : layers)
    {
        const ImVec2 a{min.x - layer.expand + layer.dx, min.y - layer.expand + layer.dy};
        const ImVec2 b{max.x + layer.expand + layer.dx, max.y + layer.expand + layer.dy};

        list.AddRectFilled(a, b, withAlphaScale(layer.colour, intensity), layer.radius);
    }
}

void fillBevelled(ImDrawList& list, ImVec2 min, ImVec2 max, float radius, ImU32 fill, bool raised)
{
    const ImU32 top = raised ? colour::biseauClair : colour::creuxHaut;
    const ImU32 bottom = raised ? colour::biseauSombre : colour::filet;

    // 1. le liseré du haut occupe tout le rectangle, il sera recouvert
    list.AddRectFilled(min, max, top, radius);

    // 2. le liseré du bas, décalé d'un pixel vers le bas
    list.AddRectFilled(ImVec2{min.x, min.y + 1.0f}, max, bottom, radius);

    // 3. le corps, qui laisse dépasser un pixel de chaque liseré
    list.AddRectFilled(ImVec2{min.x, min.y + 1.0f}, ImVec2{max.x, max.y - 1.0f}, fill, radius);
}

void applyStyle(float scale)
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.ChildRounding = metric::ray14;
    style.FrameRounding = metric::ray8;
    style.PopupRounding = metric::ray10;
    style.GrabRounding = metric::ray6;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding = metric::ray8;

    style.WindowPadding = ImVec2{metric::esp20, metric::esp20};
    style.FramePadding = ImVec2{metric::esp12, metric::esp8};
    style.ItemSpacing = ImVec2{metric::esp12, metric::esp10};
    style.ItemInnerSpacing = ImVec2{metric::esp8, metric::esp8};
    style.CellPadding = ImVec2{metric::esp8, metric::esp4};
    style.ScrollbarSize = metric::esp8;
    style.GrabMinSize = metric::esp12;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.SeparatorTextBorderSize = 1.0f;

    style.WindowTitleAlign = ImVec2{0.0f, 0.5f};
    style.ButtonTextAlign = ImVec2{0.5f, 0.5f};
    style.SelectableTextAlign = ImVec2{0.0f, 0.5f};

    ImVec4* colours = style.Colors;

    colours[ImGuiCol_WindowBg] = toVec4(colour::base);
    colours[ImGuiCol_ChildBg] = toVec4(colour::carte);
    colours[ImGuiCol_PopupBg] = toVec4(colour::carteHaute);

    colours[ImGuiCol_Text] = toVec4(colour::encre);
    colours[ImGuiCol_TextDisabled] = toVec4(colour::tertiaire);

    // Bordure décorative par défaut. Les contrôles reçoivent la bordure à fort
    // contraste au cas par cas : c'est cette différence qui dit à l'utilisateur
    // ce qui répond au clic.
    colours[ImGuiCol_Border] = toVec4(colour::filet);
    colours[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};

    colours[ImGuiCol_FrameBg] = toVec4(colour::creux);
    colours[ImGuiCol_FrameBgHovered] = toVec4(colour::controle);
    colours[ImGuiCol_FrameBgActive] = toVec4(colour::carteHaute);

    colours[ImGuiCol_Button] = toVec4(colour::controle);
    colours[ImGuiCol_ButtonHovered] = toVec4(colour::carteHaute);
    colours[ImGuiCol_ButtonActive] = toVec4(colour::creux);

    colours[ImGuiCol_Header] = toVec4(IM_COL32(0x12, 0x33, 0x22, 255));
    colours[ImGuiCol_HeaderHovered] = toVec4(IM_COL32(0x1D, 0x2A, 0x23, 255));
    colours[ImGuiCol_HeaderActive] = toVec4(IM_COL32(0x12, 0x33, 0x22, 255));

    colours[ImGuiCol_CheckMark] = toVec4(colour::accent);
    colours[ImGuiCol_SliderGrab] = toVec4(colour::accent);
    colours[ImGuiCol_SliderGrabActive] = toVec4(colour::accentClair);

    colours[ImGuiCol_Separator] = toVec4(colour::filet);
    colours[ImGuiCol_SeparatorHovered] = toVec4(colour::filetFort);
    colours[ImGuiCol_SeparatorActive] = toVec4(colour::accentFonce);

    colours[ImGuiCol_ScrollbarBg] = toVec4(colour::creux);
    colours[ImGuiCol_ScrollbarGrab] = toVec4(colour::filetFort);
    colours[ImGuiCol_ScrollbarGrabHovered] = toVec4(colour::bordControle);
    colours[ImGuiCol_ScrollbarGrabActive] = toVec4(colour::bordControle);

    colours[ImGuiCol_NavCursor] = toVec4(colour::accentClair);
    colours[ImGuiCol_ModalWindowDimBg] = ImVec4{0.0f, 0.0f, 0.0f, 168.0f / 255.0f};

    // Mise à l'échelle DPI en dernier : elle multiplie les métriques posées
    // ci-dessus. Les épaisseurs de trait sont ramenées à un pixel entier au
    // minimum, un trait de 0,7 px disparaissant par endroits et faisant
    // clignoter les bordures au redimensionnement.
    style.ScaleAllSizes(scale);

    style.ChildBorderSize = std::max(1.0f, std::round(scale));
    style.FrameBorderSize = std::max(1.0f, std::round(scale));
    style.PopupBorderSize = std::max(1.0f, std::round(scale));
}

} // namespace deuca::ui::theme
