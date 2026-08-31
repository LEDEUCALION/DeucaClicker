#pragma once

#include <memory>
#include <string>

namespace deuca::ui
{

/// Possède la fenêtre de l'application, la chaîne d'échange D3D11 et le
/// contexte Dear ImGui, et les expose sous forme de boucle de rendu.
///
/// Les types Win32 et Direct3D restent derrière un pimpl à dessein : un
/// panneau qui dessine de l'interface n'a aucune raison de voir un HWND, et
/// les garder hors de cet en-tête signifie qu'ajouter un panneau ne traîne
/// jamais <Windows.h> dans une unité de traduction de plus.
class ImGuiHost
{
public:
    struct Config
    {
        std::wstring title{L"DeucaClicker"};
        int width{760};
        int height{520};
    };

    explicit ImGuiHost(const Config& config);
    ~ImGuiHost();

    ImGuiHost(const ImGuiHost&) = delete;
    ImGuiHost& operator=(const ImGuiHost&) = delete;
    ImGuiHost(ImGuiHost&&) = delete;
    ImGuiHost& operator=(ImGuiHost&&) = delete;

    /// False si la fenêtre ou le périphérique n'ont pas pu être créés. À
    /// vérifier avant d'entrer dans la boucle de rendu ; le constructeur ne
    /// lève pas d'exception.
    [[nodiscard]] bool isValid() const noexcept;

    /// Vide la file de messages. Renvoie false dès que l'utilisateur a demandé
    /// à quitter.
    [[nodiscard]] bool pumpMessages();

    /// Démarre une image. Renvoie false tant que la fenêtre est réduite ou
    /// masquée, auquel cas l'appelant doit se mettre en attente plutôt que de
    /// tourner à vide sur une chaîne d'échange qui ne sera pas présentée.
    [[nodiscard]] bool beginFrame();

    /// Effectue le rendu de l'image et la présente à l'écran.
    void endFrame();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace deuca::ui
