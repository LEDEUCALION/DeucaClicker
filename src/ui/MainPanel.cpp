#include "ui/MainPanel.hpp"

#include "core/Version.hpp"
#include "ui/AppController.hpp"
#include "ui/Theme.hpp"
#include "ui/Widgets.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <string>

namespace deuca::ui
{
namespace
{

using namespace theme;

constexpr std::array kFunctionKeyNames{"F1", "F2", "F3", "F4",  "F5",  "F6",
                                       "F7", "F8", "F9", "F10", "F11", "F12"};

/// VK_F1 vaut 0x70 et les suivantes se succèdent.
constexpr std::uint32_t kFirstFunctionKey = 0x70;

constexpr float kMargin = metric::esp16;
constexpr float kGutter = metric::esp12;
constexpr float kCardPadX = metric::esp14;
constexpr float kCardPadY = metric::esp12;

/// Largeur de la colonne de droite.
///
/// En dessous du minimum, le bouton principal et la liste de points deviennent
/// illisibles. C'est donc la colonne de gauche qui se serre quand la fenêtre
/// rétrécit, jamais celle-ci.
constexpr float kRightColumnMin = 240.0f;
constexpr float kRightColumnPreferred = 276.0f;

/// Côté du bouton de suppression d'un point.
constexpr float kDeleteSide = 20.0f;

[[nodiscard]] ImVec4 vec4(ImU32 packed) noexcept
{
    return ImGui::ColorConvertU32ToFloat4(packed);
}

void textColoured(ImU32 packed, const char* format, ...) IM_FMTARGS(2);

void textColoured(ImU32 packed, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    ImGui::PushStyleColor(ImGuiCol_Text, vec4(packed));
    ImGui::TextV(format, args);
    ImGui::PopStyleColor();
    va_end(args);
}

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
    text += index < kFunctionKeyNames.size() ? kFunctionKeyNames[index] : "?";

    return text;
}

// ---------------------------------------------------------------------------
// Cartes
// ---------------------------------------------------------------------------

/// Ouvre une carte : fond dessiné, puis contenu dans une fenêtre fille.
///
/// La fenêtre fille est le point important. Positionner les cartes en
/// coordonnées absolues tout en laissant leur contenu s'écouler dans le flux
/// automatique fait se battre les deux : les libellés se posent où le flux les
/// mène, pas où la carte les attend, et l'ensemble se chevauche. À l'intérieur
/// d'une fille, le flux repart de zéro et redevient prévisible.
void beginCard(ImVec2 size, const char* id, const char* title, bool alert = false)
{
    ImDrawList& list = *ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max{min.x + size.x, min.y + size.y};

    drawShadow(list, min, max, kCardShadow);
    fillBevelled(list, min, max, metric::ray14, colour::carte, true);
    list.AddRect(min, max, alert ? colour::alerteBord : colour::filet, metric::ray14, 0, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{kCardPadX, kCardPadY});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});

    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    if (title != nullptr)
    {
        textColoured(alert ? colour::alerte : colour::sourdine, "%s", title);
        ImGui::Separator();
    }
}

void endCard()
{
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

/// Le bouton principal, seul aplat de couleur pleine de l'application.
///
/// L'ordre de dessin est contraint : ombre noire, puis lueur, puis
/// remplissage. Poser la lueur avant l'ombre l'éteindrait, la pile noire se
/// peignant par-dessus.
[[nodiscard]] bool primaryButton(ImVec2 size, const char* label, const char* hint, bool running, bool enabled)
{
    ImGui::PushID(label);

    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##principal", size);

    const bool pressed = enabled && ImGui::IsItemActivated();
    const bool hovered = enabled && ImGui::IsItemHovered();
    const bool held = enabled && ImGui::IsItemActive();

    const float sink = held ? 1.0f : 0.0f;
    const ImVec2 bodyMin{min.x, min.y + sink};
    const ImVec2 bodyMax{min.x + size.x, min.y + size.y + sink};

    ImU32 fill = colour::accent;
    ImU32 text = colour::encreSurAccent;
    float glow = 0.6f;

    if (!enabled)
    {
        fill = colour::controle;
        text = colour::eteintLisible;
        glow = 0.0f;
    }
    else if (running)
    {
        fill = colour::dangerPlein;
        text = colour::encreSurAplat;
        // Pulsation lente : le bouton respire tant qu'une session tourne, ce
        // qui reste perceptible du coin de l'œil sans capter l'attention.
        glow = 0.7f + 0.3f * std::sin(2.0f * 3.14159265f * 0.8f * static_cast<float>(ImGui::GetTime()));
    }
    else if (hovered)
    {
        fill = colour::accentClair;
        glow = 1.0f;
    }

    ImDrawList& list = *ImGui::GetWindowDrawList();
    drawShadow(list, bodyMin, bodyMax, kPrimaryShadow, held ? 0.5f : 1.0f);
    if (glow > 0.0f)
    {
        drawShadow(list, bodyMin, bodyMax, kGlowShadow, glow);
    }
    fillBevelled(list, bodyMin, bodyMax, metric::ray12, fill, !held);

    const ImVec2 labelSize = ImGui::CalcTextSize(label);
    const ImVec2 hintSize = hint != nullptr ? ImGui::CalcTextSize(hint) : ImVec2{0.0f, 0.0f};
    const float block = labelSize.y + (hint != nullptr ? hintSize.y + 2.0f : 0.0f);
    const float top = bodyMin.y + (size.y - block) * 0.5f;

    list.AddText(ImVec2{bodyMin.x + (size.x - labelSize.x) * 0.5f, top}, text, label);
    if (hint != nullptr)
    {
        list.AddText(ImVec2{bodyMin.x + (size.x - hintSize.x) * 0.5f, top + labelSize.y + 2.0f},
                     enabled ? text : colour::eteint, hint);
    }

    ImGui::PopID();
    return pressed;
}

// ---------------------------------------------------------------------------
// Sections
// ---------------------------------------------------------------------------

void drawStatusBar(AppController& controller, PanelState& state, ImVec2 size)
{
    const bool running = controller.isRunning();
    const bool armed = controller.panicHotkeyActive();

    beginCard(size, "##bandeau", nullptr, !armed);

    ImDrawList& list = *ImGui::GetWindowDrawList();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 lamp{cursor.x + 5.0f, cursor.y + ImGui::GetTextLineHeight() * 0.5f};

    if (running)
    {
        list.AddCircleFilled(lamp, 9.0f, IM_COL32(0x39, 0xFF, 0x14, 26), 20);
        list.AddCircleFilled(lamp, 6.0f, IM_COL32(0x39, 0xFF, 0x14, 40), 20);
    }
    list.AddCircleFilled(lamp, 4.0f, running ? colour::lueur : colour::eteint, 20);

    ImGui::Dummy(ImVec2{metric::esp12, 0.0f});
    ImGui::SameLine();
    textColoured(running ? colour::encre : colour::sourdine, running ? "EN MARCHE" : "À L'ARRÊT");

    ImGui::SameLine(0.0f, metric::esp12);
    textColoured(colour::sourdine, "%s", describeHotkey(controller.panicHotkey()).c_str());

    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - kCardPadX - 92.0f));
    if (ImGui::Button("Raccourci", ImVec2{92.0f, 0.0f}))
    {
        state.settingsOpen = true;
    }

    // La seconde rangée porte soit la télémétrie, soit l'avertissement. Un état
    // d'alerte ne déplace donc rien : il n'y a rien à mesurer quand on ne peut
    // pas démarrer.
    if (!armed)
    {
        textColoured(colour::alerte, "Raccourci d'arrêt indisponible — une autre application le détient.");
        endCard();
        return;
    }

    const EngineSnapshot snapshot = controller.snapshot();
    const auto latencyMs = std::chrono::duration<double, std::milli>{controller.governorLatency()}.count();
    const double scale = controller.governorScale() * 100.0;

    textColoured(colour::sourdine, "LOTS");
    ImGui::SameLine(0.0f, metric::esp8);
    textColoured(colour::encre, "%llu", static_cast<unsigned long long>(snapshot.burstsSubmitted));
    ImGui::SameLine(0.0f, metric::esp16);

    textColoured(colour::sourdine, "CLICS");
    ImGui::SameLine(0.0f, metric::esp8);
    textColoured(colour::encre, "%llu", static_cast<unsigned long long>(snapshot.clicksEmitted));
    ImGui::SameLine(0.0f, metric::esp16);

    textColoured(colour::sourdine, "LATENCE");
    ImGui::SameLine(0.0f, metric::esp8);
    textColoured(colour::encre, "%.1f ms", latencyMs);
    ImGui::SameLine(0.0f, metric::esp16);

    textColoured(colour::sourdine, "CADENCE");
    ImGui::SameLine(0.0f, metric::esp8);
    textColoured(scale < 99.0 ? colour::alerte : colour::accentClair, "%.0f %%", scale);

    endCard();
}

void drawIntervalCard(AppController& controller, ImVec2 size)
{
    beginCard(size, "##intervalle", "INTERVALLE ENTRE CLICS");

    ClickPlan& plan = controller.plan();
    IntervalFields& interval = controller.interval();
    bool changed = false;

    // Un tableau plutôt que des SameLine à décalage manuel : les quatre
    // colonnes se partagent la largeur disponible, et chaque libellé tombe
    // sous son champ quelle que soit la taille de la fenêtre.
    if (ImGui::BeginTable("##champs", 4, ImGuiTableFlags_SizingStretchSame))
    {
        constexpr std::array labels{"H", "MIN", "S", "MS"};
        const std::array<int*, 4> values{&interval.hours, &interval.minutes, &interval.seconds,
                                         &interval.milliseconds};

        ImGui::TableNextRow();
        for (int column = 0; column < 4; ++column)
        {
            ImGui::TableSetColumnIndex(column);
            ImGui::PushID(column);

            ImGui::SetNextItemWidth(-FLT_MIN);
            const bool edited = ImGui::InputInt("##valeur", values[static_cast<std::size_t>(column)], 0);
            changed = edited || changed;
            textColoured(colour::tertiaire, "%s", labels[static_cast<std::size_t>(column)]);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    bool jittered = plan.jitter > Duration::zero();
    if (ImGui::Checkbox("Variation ±", &jittered))
    {
        plan.jitter = jittered ? Duration{std::chrono::milliseconds{20}} : Duration::zero();
    }

    ImGui::SameLine(0.0f, metric::esp8);
    ImGui::BeginDisabled(!jittered);
    int jitterMs =
        static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(plan.jitter).count());
    ImGui::SetNextItemWidth(56.0f);
    if (ImGui::InputInt("ms##jitter", &jitterMs, 0) && jittered)
    {
        plan.jitter = Duration{std::chrono::milliseconds{std::max(0, jitterMs)}};
    }
    ImGui::EndDisabled();

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

    const auto effectiveMs =
        std::chrono::duration<double, std::milli>{controller.effectiveInterval()}.count();
    textColoured(colour::tertiaire, "%.1f %s/s · appliqué %.2f ms", plan.clicksPerSecond,
                 plan.style == ClickStyle::Double ? "doubles-clics" : "clics", effectiveMs);

    endCard();
}

void drawOptionsCard(AppController& controller, ImVec2 size)
{
    beginCard(size, "##options", "OPTIONS DE CLIC");

    ClickPlan& plan = controller.plan();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##bouton", buttonName(plan.button)))
    {
        for (const MouseButton candidate : {MouseButton::Left, MouseButton::Right, MouseButton::Middle})
        {
            if (ImGui::Selectable(buttonName(candidate), plan.button == candidate))
            {
                plan.button = candidate;
            }
        }
        ImGui::EndCombo();
    }

    const bool isDouble = plan.style == ClickStyle::Double;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##type", isDouble ? "Double clic" : "Clic simple"))
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

    bool grouped = plan.burstSize > 1;
    if (ImGui::Checkbox("Grouper les clics", &grouped))
    {
        plan.burstSize = grouped ? 16 : 1;
    }

    ImGui::BeginDisabled(!grouped);
    int burst = static_cast<int>(plan.burstSize);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderInt("##lot", &burst, 1, 128, "%d par lot") && grouped)
    {
        plan.burstSize = static_cast<std::size_t>(std::max(1, burst));
    }
    ImGui::EndDisabled();

    endCard();
}

void drawRepeatCard(AppController& controller, ImVec2 size)
{
    beginCard(size, "##repetition", "RÉPÉTITION");

    ClickPlan& plan = controller.plan();
    bool limited = plan.repeatLimit > 0;

    if (ImGui::RadioButton("Jusqu'à l'arrêt", !limited))
    {
        plan.repeatLimit = 0;
        limited = false;
    }

    if (ImGui::RadioButton("Un nombre de fois", limited))
    {
        // Une valeur de départ plutôt que zéro : basculer sur ce mode avec une
        // limite nulle donnerait un moteur qui refuse de démarrer sans raison
        // visible.
        plan.repeatLimit = plan.repeatLimit > 0 ? plan.repeatLimit : 100;
        limited = true;
    }

    ImGui::BeginDisabled(!limited);
    int repeats = static_cast<int>(std::min<std::uint64_t>(plan.repeatLimit, INT_MAX));
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputInt("##repetitions", &repeats, 1, 100) && limited)
    {
        plan.repeatLimit = static_cast<std::uint64_t>(std::max(1, repeats));
    }
    ImGui::EndDisabled();

    if (limited && controller.isRunning())
    {
        const std::uint64_t done = controller.snapshot().clicksEmitted;
        const std::uint64_t left = plan.repeatLimit > done ? plan.repeatLimit - done : 0;
        textColoured(colour::accentClair, "Restant : %llu", static_cast<unsigned long long>(left));
    }

    endCard();
}

void drawTargetsCard(AppController& controller, ImVec2 size)
{
    beginCard(size, "##cibles", "POSITION DU CURSEUR");

    ClickPlan& plan = controller.plan();
    bool useTargets = controller.useTargets();

    // Les deux modes restent toujours sélectionnables. Griser « points
    // enregistrés » tant que la liste est vide enfermait l'utilisateur : le
    // mode était inaccessible avant d'avoir capturé, et capturer semblait
    // supposer d'être déjà dans le mode.
    if (ImGui::RadioButton("Position actuelle", !useTargets))
    {
        controller.setUseTargets(false);
    }
    if (ImGui::RadioButton("Points enregistrés", useTargets))
    {
        controller.setUseTargets(true);
    }

    // Le puits occupe ce qui reste une fois la rangée de boutons réservée.
    //
    // La réserve compte la hauteur du bouton **et** les deux espacements qui
    // l'encadrent. En n'en comptant qu'un, la rangée débordait du bas de la
    // carte et les libellés se faisaient trancher — un bouton dont on ne lit
    // que la moitié haute n'est plus un bouton.
    const float buttonRow = metric::esp32 + ImGui::GetStyle().ItemSpacing.y * 2.0f;
    const float wellHeight = std::max(48.0f, ImGui::GetContentRegionAvail().y - buttonRow);
    const ImVec2 wellSize{ImGui::GetContentRegionAvail().x, wellHeight};
    const ImVec2 wellMin = ImGui::GetCursorScreenPos();

    fillBevelled(*ImGui::GetWindowDrawList(), wellMin, ImVec2{wellMin.x + wellSize.x, wellMin.y + wellSize.y},
                 metric::ray10, colour::creux, false);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{metric::esp8, metric::esp8});
    ImGui::BeginChild("##points", wellSize, ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoBackground);

    std::size_t toRemove = plan.targets.size();

    if (controller.capturingPoint())
    {
        // Le cas se voit surtout si l'utilisateur remonte la fenêtre pendant la
        // désignation. Le guide principal reste le repère qui suit le curseur.
        const ScreenPoint live = controller.liveCursor();
        textColoured(colour::accentClair, "Désignation en cours");
        textColoured(colour::encre, "X %d  Y %d", live.x, live.y);
        textColoured(colour::tertiaire, "Clic gauche : enregistrer");
        textColoured(colour::tertiaire, "Clic droit : annuler");
    }
    else if (plan.targets.empty())
    {
        textColoured(colour::eteint, "Aucun point.");
        textColoured(colour::eteint, "Utilisez Désigner pour");
        textColoured(colour::eteint, "en enregistrer un.");
    }
    else
    {
        for (std::size_t i = 0; i < plan.targets.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            const float rowTop = ImGui::GetCursorPosY();

            textColoured(colour::accentClair, "%2zu", i + 1);
            ImGui::SameLine(0.0f, metric::esp8);
            textColoured(colour::encre, "X %-5d Y %-5d", plan.targets[i].x, plan.targets[i].y);

            // La croix est calée à droite et centrée sur la ligne plutôt que
            // posée à la suite du texte : la position d'un bouton de
            // suppression ne doit pas dépendre de la longueur des coordonnées
            // qu'il accompagne.
            ImGui::SetCursorPos(ImVec2{ImGui::GetContentRegionMax().x - kDeleteSide, rowTop});
            if (deleteButton("suppr", kDeleteSide))
            {
                toRemove = i;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Supprimer ce point");
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (toRemove < plan.targets.size())
    {
        controller.removeTarget(toRemove);
    }

    const float half = (ImGui::GetContentRegionAvail().x - metric::esp8) * 0.5f;

    // Les deux boutons portent leur intention par la couleur. Ajouter et
    // détruire côte à côte, du même gris, est la disposition qui produit les
    // suppressions accidentelles.
    //
    // Désigner efface l'application le temps du geste : c'est le seul déroulé
    // qui fonctionne, capturer la position courante obligeant à garder la
    // fenêtre sous les yeux tout en visant ailleurs.
    if (toneButton("Désigner", ImVec2{half, metric::esp32}, ButtonTone::Accent))
    {
        controller.beginPointCapture();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("L'application s'efface le temps du geste.\n"
                          "Clic gauche : enregistrer le point.\n"
                          "Clic droit : annuler.");
    }

    ImGui::SameLine(0.0f, metric::esp8);

    const bool hasTargets = !plan.targets.empty();
    if (toneButton("Vider", ImVec2{half, metric::esp32}, ButtonTone::Danger, hasTargets))
    {
        controller.clearTargets();
        controller.setUseTargets(false);
    }
    if (hasTargets && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Supprimer les %zu points", plan.targets.size());
    }

    endCard();
}

void drawSettingsModal(AppController& controller, PanelState& state)
{
    if (state.settingsOpen && !ImGui::IsPopupOpen("Raccourci"))
    {
        ImGui::OpenPopup("Raccourci");
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                   viewport->WorkPos.y + viewport->WorkSize.y * 0.5f},
                            ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    if (!ImGui::BeginPopupModal("Raccourci", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::Checkbox("Ctrl", &state.pendingControl);
    ImGui::SameLine(0.0f, metric::esp16);
    ImGui::Checkbox("Alt", &state.pendingAlt);
    ImGui::SameLine(0.0f, metric::esp16);
    ImGui::Checkbox("Maj", &state.pendingShift);

    ImGui::SetNextItemWidth(120.0f);
    ImGui::Combo("Touche", &state.pendingFunctionKeyIndex, kFunctionKeyNames.data(),
                 static_cast<int>(kFunctionKeyNames.size()));

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

    const platform::Hotkey candidate{.modifiers = modifiers,
                                     .virtualKey = kFirstFunctionKey +
                                                   static_cast<std::uint32_t>(state.pendingFunctionKeyIndex)};

    ImGui::Spacing();
    textColoured(colour::accentClair, "%s", describeHotkey(candidate).c_str());

    // La place de l'avertissement est réservée en permanence : la modale garde
    // sa hauteur, qu'un refus survienne ou non.
    if (state.rebindFailed)
    {
        textColoured(colour::alerte, "Refusée : une autre application la détient.");
    }
    else
    {
        ImGui::Dummy(ImVec2{0.0f, ImGui::GetTextLineHeight()});
    }

    ImGui::Spacing();

    if (ImGui::Button("Annuler", ImVec2{110.0f, metric::esp32}))
    {
        state.settingsOpen = false;
        state.rebindFailed = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine(0.0f, metric::esp12);
    if (ImGui::Button("Appliquer", ImVec2{110.0f, metric::esp32}))
    {
        state.rebindFailed = !controller.rebindPanicHotkey(candidate);
        if (!state.rebindFailed)
        {
            state.settingsOpen = false;
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}

} // namespace

void drawMainPanel(AppController& controller, PanelState& state)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{kMargin, kMargin});
    ImGui::Begin("DeucaClicker", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

    // Toutes les dimensions se déduisent de la fenêtre réelle. Un gabarit figé
    // déborde dès qu'on redimensionne, et ImGui le signale alors par une
    // assertion plutôt que par un simple défaut d'affichage.
    const float totalW = ImGui::GetContentRegionAvail().x;
    const float totalH = ImGui::GetContentRegionAvail().y;

    const float footer = ImGui::GetTextLineHeight() + metric::esp4;
    const float rightW = std::clamp(kRightColumnPreferred, kRightColumnMin, totalW * 0.45f);
    const float leftW = totalW - rightW - kGutter;

    const float bandH = ImGui::GetTextLineHeightWithSpacing() * 2.0f + kCardPadY * 2.0f + metric::esp4;
    const float bodyH = totalH - bandH - kGutter - footer - kGutter;

    // Hauteur minimale de la rangée basse, déduite de ce qu'elle doit contenir
    // et non choisie à vue : titre, filet, deux listes déroulantes, une case à
    // cocher et un curseur, plus les marges de la carte. Sous cette valeur, le
    // contenu se fait couper — c'est ce qui arrivait avec un partage en
    // pourcentage, qui ignore ce qu'il y a dans les cartes.
    const float row = ImGui::GetFrameHeightWithSpacing();
    const float lowerMin = ImGui::GetTextLineHeightWithSpacing() + row * 4.0f + kCardPadY * 2.0f;

    const float primaryH = metric::esp56;
    const float intervalH = std::clamp(bodyH - lowerMin - kGutter, 120.0f, 170.0f);
    const float lowerH = bodyH - intervalH - kGutter;

    const ImVec2 origin = ImGui::GetCursorPos();
    const float bodyY = origin.y + bandH + kGutter;

    drawStatusBar(controller, state, ImVec2{totalW, bandH});

    ImGui::SetCursorPos(ImVec2{origin.x, bodyY});
    drawIntervalCard(controller, ImVec2{leftW, intervalH});

    ImGui::SetCursorPos(ImVec2{origin.x, bodyY + intervalH + kGutter});
    const float halfLeft = (leftW - kGutter) * 0.5f;
    drawOptionsCard(controller, ImVec2{halfLeft, lowerH});
    ImGui::SameLine(0.0f, kGutter);
    drawRepeatCard(controller, ImVec2{halfLeft, lowerH});

    // Le bouton principal est en haut à droite, à la même hauteur que la carte
    // d'intervalle : le regard le rencontre au terme du premier balayage
    // horizontal, et sa position ne bouge jamais.
    const bool running = controller.isRunning();
    const bool armed = controller.panicHotkeyActive();
    const std::string hint = describeHotkey(controller.panicHotkey());

    ImGui::SetCursorPos(ImVec2{origin.x + leftW + kGutter, bodyY});
    if (primaryButton(ImVec2{rightW, primaryH}, running ? "ARRÊTER" : "DÉMARRER", hint.c_str(), running,
                      armed))
    {
        controller.toggle();
    }

    ImGui::SetCursorPos(ImVec2{origin.x + leftW + kGutter, bodyY + primaryH + kGutter});
    drawTargetsCard(controller, ImVec2{rightW, bodyH - primaryH - kGutter});

    ImGui::SetCursorPos(ImVec2{origin.x, bodyY + bodyH + kGutter});
    textColoured(colour::eteint, "%s", deuca::buildBanner().c_str());

    drawSettingsModal(controller, state);

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace deuca::ui
