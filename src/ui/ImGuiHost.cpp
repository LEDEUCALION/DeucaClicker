#include "ui/ImGuiHost.hpp"

#include "platform/WindowsLean.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>
#include <dxgi.h>

#include <iterator>

// Déclarée par le backend mais délibérément absente de son en-tête public, afin
// que les applications aient à choisir explicitement de lui transmettre les
// messages.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace deuca::ui
{
namespace
{

constexpr wchar_t kWindowClassName[] = L"DeucaClicker.MainWindow";
constexpr float kDefaultDpi = 96.0f;

/// Libère une interface COM et remet le pointeur à nul, dans cet ordre.
template <typename T>
void safeRelease(T*& resource) noexcept
{
    if (resource != nullptr)
    {
        resource->Release();
        resource = nullptr;
    }
}

/// Replace la fenêtre sur le rectangle que Windows propose dans WM_DPICHANGED.
///
/// Ignorer ce rectangle, c'est ce qui fait qu'une application se retrouve à la
/// mauvaise taille quand on la fait glisser entre deux écrans dont la mise à
/// l'échelle diffère.
void applySuggestedDpiRect(HWND hwnd, LPARAM lParam) noexcept
{
    const auto* suggested = reinterpret_cast<const RECT*>(lParam);
    ::SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                   suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
}

} // namespace

struct ImGuiHost::Impl
{
    HWND window{nullptr};
    ATOM windowClass{0};
    HINSTANCE instance{nullptr};

    ID3D11Device* device{nullptr};
    ID3D11DeviceContext* deviceContext{nullptr};
    IDXGISwapChain* swapChain{nullptr};
    ID3D11RenderTargetView* backBufferView{nullptr};

    bool imguiReady{false};
    bool quitRequested{false};

    // WM_SIZE arrive en rafale tant que l'utilisateur fait glisser le bord de
    // la fenêtre. Recréer les tampons de la chaîne d'échange à chacun de ces
    // messages gaspille du travail et provoque un scintillement : on note donc
    // la demande et on l'applique une seule fois, en tête de l'image suivante.
    UINT pendingWidth{0};
    UINT pendingHeight{0};

    bool createDevice();
    void destroyDevice();
    void createBackBufferView();
    void destroyBackBufferView();
    void applyPendingResize();

    static LRESULT CALLBACK windowProc(HWND, UINT, WPARAM, LPARAM);
};

bool ImGuiHost::Impl::createDevice()
{
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL requested[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtained{};

    // Une interface aussi légère n'a aucune raison d'exiger une carte dédiée ;
    // le repli sur WARP garde l'outil utilisable dans une machine virtuelle et
    // en session distante.
    for (const D3D_DRIVER_TYPE driver : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP})
    {
        const HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
            nullptr, driver, nullptr, 0, requested, static_cast<UINT>(std::size(requested)),
            D3D11_SDK_VERSION, &desc, &swapChain, &device, &obtained, &deviceContext);

        if (SUCCEEDED(hr))
        {
            createBackBufferView();
            return true;
        }
    }

    return false;
}

void ImGuiHost::Impl::destroyDevice()
{
    destroyBackBufferView();
    safeRelease(swapChain);
    safeRelease(deviceContext);
    safeRelease(device);
}

void ImGuiHost::Impl::createBackBufferView()
{
    ID3D11Texture2D* backBuffer{nullptr};
    if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) && backBuffer != nullptr)
    {
        device->CreateRenderTargetView(backBuffer, nullptr, &backBufferView);
        backBuffer->Release();
    }
}

void ImGuiHost::Impl::destroyBackBufferView()
{
    safeRelease(backBufferView);
}

void ImGuiHost::Impl::applyPendingResize()
{
    if (pendingWidth == 0 || pendingHeight == 0)
    {
        return;
    }

    destroyBackBufferView();
    swapChain->ResizeBuffers(0, pendingWidth, pendingHeight, DXGI_FORMAT_UNKNOWN, 0);
    createBackBufferView();

    pendingWidth = 0;
    pendingHeight = 0;
}

LRESULT CALLBACK ImGuiHost::Impl::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui a un droit de refus sur chaque message : il lui faut ceux qu'il
    // consomme (capture de la souris, IME, changements de focus) avant que nous
    // ne décidions quoi que ce soit.
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
    {
        return 1;
    }

    auto* self = reinterpret_cast<Impl*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_SIZE:
        if (self != nullptr && wParam != SIZE_MINIMIZED)
        {
            self->pendingWidth = LOWORD(lParam);
            self->pendingHeight = HIWORD(lParam);
        }
        return 0;

    case WM_DPICHANGED:
        applySuggestedDpiRect(hwnd, lParam);
        return 0;

    case WM_SYSCOMMAND:
        // On avale l'activation du menu par Alt et F10 : il n'y a ici aucun
        // menu système à ouvrir, et cela ne fait que voler le focus clavier à
        // l'interface.
        if ((wParam & 0xFFF0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_CLOSE:
        if (self != nullptr)
        {
            self->quitRequested = true;
        }
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

ImGuiHost::ImGuiHost(const Config& config) : m_impl{std::make_unique<Impl>()}
{
    m_impl->instance = ::GetModuleHandleW(nullptr);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &Impl::windowProc;
    windowClass.hInstance = m_impl->instance;
    windowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;

    m_impl->windowClass = ::RegisterClassExW(&windowClass);
    if (m_impl->windowClass == 0)
    {
        return;
    }

    m_impl->window = ::CreateWindowExW(0, kWindowClassName, config.title.c_str(), WS_OVERLAPPEDWINDOW,
                                       CW_USEDEFAULT, CW_USEDEFAULT, config.width, config.height, nullptr,
                                       nullptr, m_impl->instance, nullptr);
    if (m_impl->window == nullptr)
    {
        return;
    }

    ::SetWindowLongPtrW(m_impl->window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m_impl.get()));

    if (!m_impl->createDevice())
    {
        m_impl->destroyDevice();
        return;
    }

    const float dpiScale = static_cast<float>(::GetDpiForWindow(m_impl->window)) / kDefaultDpi;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Le multi-fenêtrage reste désactivé : l'outil tient dans une seule fenêtre
    // compacte, et des vues détachées réclameraient chacune leur propre chaîne
    // d'échange sans rien apporter.
    io.ConfigDpiScaleFonts = true;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(dpiScale);

    if (!ImGui_ImplWin32_Init(m_impl->window) || !ImGui_ImplDX11_Init(m_impl->device, m_impl->deviceContext))
    {
        ImGui::DestroyContext();
        m_impl->destroyDevice();
        return;
    }

    m_impl->imguiReady = true;

    ::ShowWindow(m_impl->window, SW_SHOWDEFAULT);
    ::UpdateWindow(m_impl->window);
}

ImGuiHost::~ImGuiHost()
{
    // Ordre strictement inverse de la construction : les backends détiennent
    // des objets du périphérique, ils doivent donc les relâcher avant que le
    // périphérique lui-même ne disparaisse.
    if (m_impl->imguiReady)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    m_impl->destroyDevice();

    if (m_impl->window != nullptr)
    {
        ::DestroyWindow(m_impl->window);
    }

    if (m_impl->windowClass != 0)
    {
        ::UnregisterClassW(kWindowClassName, m_impl->instance);
    }
}

bool ImGuiHost::isValid() const noexcept
{
    return m_impl->imguiReady;
}

bool ImGuiHost::pumpMessages()
{
    MSG message{};
    while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);

        if (message.message == WM_QUIT)
        {
            m_impl->quitRequested = true;
        }
    }

    return !m_impl->quitRequested;
}

bool ImGuiHost::beginFrame()
{
    if (::IsIconic(m_impl->window))
    {
        return false;
    }

    m_impl->applyPendingResize();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    return true;
}

void ImGuiHost::endFrame()
{
    ImGui::Render();

    constexpr float clearColour[4] = {0.06f, 0.07f, 0.09f, 1.0f};
    m_impl->deviceContext->OMSetRenderTargets(1, &m_impl->backBufferView, nullptr);
    m_impl->deviceContext->ClearRenderTargetView(m_impl->backBufferView, clearColour);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Présentation synchronisée à l'écran : l'interface n'a aucune raison de
    // s'afficher plus vite que le moniteur, et rien de sensible au temps ne vit
    // sur ce fil d'exécution.
    m_impl->swapChain->Present(1, 0);
}

} // namespace deuca::ui
