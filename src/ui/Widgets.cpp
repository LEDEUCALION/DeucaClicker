#include "ui/Widgets.hpp"

#include "ui/Theme.hpp"

namespace deuca::ui
{
namespace
{

using namespace theme;

/// Les trois couleurs qui définissent un bouton dans un état donné.
struct ToneColours
{
    ImU32 fill{};
    ImU32 border{};
    ImU32 text{};
};

/// Teintes de fond, volontairement sombres.
///
/// Le fond ne fait que colorer l'ombre du bouton ; c'est la bordure qui porte
/// le signal. Une bordure à fort contraste dit « cliquable », un fond teinté
/// dit seulement « de cette famille ».
constexpr ImU32 kAccentFill = IM_COL32(0x12, 0x33, 0x22, 255);
constexpr ImU32 kAccentFillHover = IM_COL32(0x18, 0x45, 0x2D, 255);
constexpr ImU32 kDangerFill = IM_COL32(0x2A, 0x0C, 0x0D, 255);
constexpr ImU32 kDangerFillHover = IM_COL32(0x3A, 0x11, 0x12, 255);

[[nodiscard]] ToneColours resolve(ButtonTone tone, bool enabled, bool hovered, bool held) noexcept
{
    if (!enabled)
    {
        // Un bouton désactivé garde sa forme mais perd sa famille : lui laisser
        // sa couleur inviterait à cliquer sur ce qui ne répond pas.
        return ToneColours{.fill = colour::creux, .border = colour::filet, .text = colour::eteintLisible};
    }

    switch (tone)
    {
    case ButtonTone::Accent:
        return ToneColours{.fill = held ? kAccentFill : (hovered ? kAccentFillHover : kAccentFill),
                           .border = hovered ? colour::accentClair : colour::accent,
                           .text = hovered ? colour::accentClair : colour::accent};

    case ButtonTone::Danger:
        return ToneColours{.fill = held ? kDangerFill : (hovered ? kDangerFillHover : kDangerFill),
                           .border = hovered ? colour::danger : colour::dangerPlein,
                           .text = hovered ? colour::danger : colour::dangerPlein};

    case ButtonTone::Neutral:
        break;
    }

    return ToneColours{.fill = hovered ? colour::carteHaute : colour::controle,
                       .border = hovered ? colour::bordControle : colour::filetFort,
                       .text = colour::encre};
}

} // namespace

bool toneButton(const char* label, ImVec2 size, ButtonTone tone, bool enabled)
{
    ImGui::PushID(label);

    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##zone", size);

    const bool pressed = enabled && ImGui::IsItemActivated();
    const bool hovered = enabled && ImGui::IsItemHovered();
    const bool held = enabled && ImGui::IsItemActive();

    // Le bouton s'enfonce d'un pixel : c'est le seul retour tactile qu'une
    // interface plate puisse offrir, et il coûte une addition.
    const float sink = held ? 1.0f : 0.0f;
    const ImVec2 bodyMin{min.x, min.y + sink};
    const ImVec2 bodyMax{min.x + size.x, min.y + size.y + sink};

    const ToneColours colours = resolve(tone, enabled, hovered, held);
    ImDrawList& list = *ImGui::GetWindowDrawList();

    if (enabled)
    {
        drawShadow(list, bodyMin, bodyMax, kSecondaryShadow, held ? 0.4f : 1.0f);
    }

    fillBevelled(list, bodyMin, bodyMax, metric::ray8, colours.fill, !held);
    list.AddRect(bodyMin, bodyMax, colours.border, metric::ray8, 0, 1.0f);

    const ImVec2 textSize = ImGui::CalcTextSize(label);
    list.AddText(ImVec2{bodyMin.x + (size.x - textSize.x) * 0.5f, bodyMin.y + (size.y - textSize.y) * 0.5f},
                 colours.text, label);

    ImGui::PopID();
    return pressed;
}

bool deleteButton(const char* id, float side)
{
    ImGui::PushID(id);

    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##suppr", ImVec2{side, side});

    const bool pressed = ImGui::IsItemActivated();
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 max{min.x + side, min.y + side};

    ImDrawList& list = *ImGui::GetWindowDrawList();

    if (hovered)
    {
        list.AddRectFilled(min, max, kDangerFillHover, metric::ray5);
        list.AddRect(min, max, colour::danger, metric::ray5, 0, 1.0f);
    }

    // La croix est tracée à la main plutôt qu'écrite en texte : deux segments
    // restent nets à toute échelle, là où un glyphe « x » de police change de
    // forme et de centrage d'une taille à l'autre.
    const float inset = side * 0.3f;
    const ImU32 glyph = hovered ? colour::danger : colour::dangerPlein;

    list.AddLine(ImVec2{min.x + inset, min.y + inset}, ImVec2{max.x - inset, max.y - inset}, glyph, 1.6f);
    list.AddLine(ImVec2{max.x - inset, min.y + inset}, ImVec2{min.x + inset, max.y - inset}, glyph, 1.6f);

    ImGui::PopID();
    return pressed;
}

} // namespace deuca::ui
