#include "platform/CaptureOverlay.hpp"

#include "platform/WindowsLean.hpp"

#include <cstdio>

namespace deuca::platform
{
namespace
{

constexpr wchar_t kClassName[] = L"DeucaClicker.CaptureOverlay";

constexpr int kWidth = 268;
constexpr int kHeight = 62;

/// Décalage du repère par rapport au curseur.
///
/// Assez loin pour ne jamais recouvrir le pixel exact que l'utilisateur vise —
/// masquer sa cible avec l'aide censée l'assister serait le comble.
constexpr int kOffsetX = 22;
constexpr int kOffsetY = 18;

constexpr COLORREF kBackground = RGB(0x10, 0x16, 0x13);
constexpr COLORREF kBorder = RGB(0x22, 0xC5, 0x5E);
constexpr COLORREF kCoordinates = RGB(0xE9, 0xEF, 0xEB);
constexpr COLORREF kHint = RGB(0x9A, 0xA8, 0xA0);

struct OverlayText
{
    wchar_t coordinates[64]{};
};

LRESULT CALLBACK overlayProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT)
    {
        const auto* text = reinterpret_cast<const OverlayText*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        PAINTSTRUCT paint{};
        const HDC dc = ::BeginPaint(hwnd, &paint);

        RECT client{};
        ::GetClientRect(hwnd, &client);

        const HBRUSH background = ::CreateSolidBrush(kBackground);
        ::FillRect(dc, &client, background);
        ::DeleteObject(background);

        const HPEN pen = ::CreatePen(PS_SOLID, 1, kBorder);
        const HGDIOBJ oldPen = ::SelectObject(dc, pen);
        const HGDIOBJ oldBrush = ::SelectObject(dc, ::GetStockObject(NULL_BRUSH));
        ::RoundRect(dc, client.left, client.top, client.right, client.bottom, 10, 10);
        ::SelectObject(dc, oldBrush);
        ::SelectObject(dc, oldPen);
        ::DeleteObject(pen);

        // Segoe UI en deux graisses : la coordonnée doit se lire d'un coup
        // d'œil, la consigne se lit une fois puis s'oublie.
        const HFONT bold =
            ::CreateFontW(-16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        const HFONT regular =
            ::CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");

        ::SetBkMode(dc, TRANSPARENT);

        RECT line{client.left + 12, client.top + 8, client.right - 12, client.top + 30};
        ::SelectObject(dc, bold);
        ::SetTextColor(dc, kCoordinates);
        ::DrawTextW(dc, text != nullptr ? text->coordinates : L"", -1, &line, DT_LEFT | DT_SINGLELINE);

        RECT hint{client.left + 12, client.top + 30, client.right - 12, client.bottom - 8};
        ::SelectObject(dc, regular);
        ::SetTextColor(dc, kHint);
        ::DrawTextW(dc, L"Clic gauche : enregistrer  ·  Clic droit : annuler", -1, &hint,
                    DT_LEFT | DT_WORDBREAK);

        ::DeleteObject(bold);
        ::DeleteObject(regular);
        ::EndPaint(hwnd, &paint);
        return 0;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

struct CaptureOverlay::Impl
{
    HWND window{nullptr};
    ATOM windowClass{0};
    OverlayText text{};
};

CaptureOverlay::CaptureOverlay() noexcept : m_impl{std::make_unique<Impl>()} {}

CaptureOverlay::~CaptureOverlay()
{
    hide();

    if (m_impl->window != nullptr)
    {
        ::DestroyWindow(m_impl->window);
        m_impl->window = nullptr;
    }

    if (m_impl->windowClass != 0)
    {
        ::UnregisterClassW(kClassName, ::GetModuleHandleW(nullptr));
    }
}

bool CaptureOverlay::show(ScreenPoint cursor)
{
    if (m_impl->window == nullptr)
    {
        const HINSTANCE instance = ::GetModuleHandleW(nullptr);

        if (m_impl->windowClass == 0)
        {
            WNDCLASSEXW windowClass{};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.lpfnWndProc = &overlayProc;
            windowClass.hInstance = instance;
            windowClass.lpszClassName = kClassName;
            m_impl->windowClass = ::RegisterClassExW(&windowClass);
        }

        // TRANSPARENT laisse passer les clics, NOACTIVATE empêche le vol de
        // focus, TOOLWINDOW garde le repère hors de la barre des tâches. Les
        // trois sont indispensables : un guide qui intercepte le clic qu'il
        // invite à faire est pire que pas de guide.
        m_impl->window = ::CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            kClassName, L"", WS_POPUP, 0, 0, kWidth, kHeight, nullptr, nullptr, instance, nullptr);
        if (m_impl->window == nullptr)
        {
            return false;
        }

        ::SetWindowLongPtrW(m_impl->window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&m_impl->text));
        ::SetLayeredWindowAttributes(m_impl->window, 0, 236, LWA_ALPHA);
    }

    moveTo(cursor);
    ::ShowWindow(m_impl->window, SW_SHOWNOACTIVATE);

    return true;
}

void CaptureOverlay::moveTo(ScreenPoint cursor)
{
    if (m_impl->window == nullptr)
    {
        return;
    }

    std::swprintf(m_impl->text.coordinates, std::size(m_impl->text.coordinates), L"X %d     Y %d", cursor.x,
                  cursor.y);

    // Le repère bascule de l'autre côté du curseur quand il sortirait de
    // l'écran. Sans cela il se ferait tronquer par le bord droit, précisément
    // là où l'on va souvent désigner quelque chose.
    const int screenRight = ::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int screenBottom = ::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

    int x = cursor.x + kOffsetX;
    int y = cursor.y + kOffsetY;

    if (x + kWidth > screenRight)
    {
        x = cursor.x - kOffsetX - kWidth;
    }
    if (y + kHeight > screenBottom)
    {
        y = cursor.y - kOffsetY - kHeight;
    }

    ::SetWindowPos(m_impl->window, HWND_TOPMOST, x, y, kWidth, kHeight, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ::InvalidateRect(m_impl->window, nullptr, TRUE);
}

void CaptureOverlay::hide()
{
    if (m_impl->window != nullptr)
    {
        ::ShowWindow(m_impl->window, SW_HIDE);
    }
}

} // namespace deuca::platform
