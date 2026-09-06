#include "platform/PointPicker.hpp"

#include "platform/WindowsLean.hpp"

#include <atomic>

namespace deuca::platform
{
namespace
{

/// Le crochet bas niveau est une fonction libre : le système l'appelle sans
/// contexte utilisateur. Il faut donc un pointeur accessible depuis elle.
///
/// Une seule capture peut être en cours à la fois — désigner deux points
/// simultanément n'aurait aucun sens — et cette variable porte cette
/// contrainte plutôt que de la laisser implicite.
struct PickerState
{
    HHOOK hook{nullptr};
    HWND window{nullptr};
    std::atomic<bool> active{false};
    std::atomic<bool> captured{false};
    std::atomic<LONG> x{0};
    std::atomic<LONG> y{0};
    std::atomic<LONG> liveX{0};
    std::atomic<LONG> liveY{0};
};

PickerState* g_active = nullptr;

LRESULT CALLBACK mouseHook(int code, WPARAM message, LPARAM data)
{
    PickerState* state = g_active;

    if (code != HC_ACTION || state == nullptr || !state->active.load(std::memory_order_acquire))
    {
        return ::CallNextHookEx(nullptr, code, message, data);
    }

    const auto* mouse = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);

    if (message == WM_MOUSEMOVE)
    {
        state->liveX.store(mouse->pt.x, std::memory_order_relaxed);
        state->liveY.store(mouse->pt.y, std::memory_order_relaxed);
        return ::CallNextHookEx(nullptr, code, message, data);
    }

    // Le clic droit annule. C'est le geste d'échappement attendu, et il évite
    // d'imposer un aller-retour au clavier alors que la main est sur la souris.
    if (message == WM_RBUTTONDOWN)
    {
        state->active.store(false, std::memory_order_release);
        return 1;
    }

    if (message == WM_LBUTTONDOWN)
    {
        state->x.store(mouse->pt.x, std::memory_order_relaxed);
        state->y.store(mouse->pt.y, std::memory_order_relaxed);
        state->captured.store(true, std::memory_order_release);
        state->active.store(false, std::memory_order_release);

        // Renvoyer une valeur non nulle consomme l'événement : il n'atteint
        // jamais l'application sous le curseur. Sans cela, désigner un bouton
        // reviendrait à l'actionner.
        return 1;
    }

    // Le relâchement qui suit un clic avalé doit l'être aussi, sinon la fenêtre
    // survolée reçoit un relâchement orphelin et peut s'en trouver perturbée.
    if (message == WM_LBUTTONUP || message == WM_RBUTTONUP)
    {
        return 1;
    }

    return ::CallNextHookEx(nullptr, code, message, data);
}

} // namespace

struct PointPicker::Impl
{
    PickerState state;
};

PointPicker::PointPicker() noexcept : m_impl{std::make_unique<Impl>()} {}

PointPicker::~PointPicker()
{
    cancel();
}

bool PointPicker::begin(void* window)
{
    if (m_impl->state.active.load(std::memory_order_acquire))
    {
        return true;
    }

    m_impl->state.captured.store(false, std::memory_order_release);
    m_impl->state.window = static_cast<HWND>(window);

    POINT current{};
    if (::GetCursorPos(&current) != FALSE)
    {
        m_impl->state.liveX.store(current.x, std::memory_order_relaxed);
        m_impl->state.liveY.store(current.y, std::memory_order_relaxed);
    }

    // Le crochet est posé avant de réduire la fenêtre : si la pose échoue, rien
    // n'a bougé et l'utilisateur ne se retrouve pas devant une application
    // disparue qui n'écoute rien.
    m_impl->state.hook = ::SetWindowsHookExW(WH_MOUSE_LL, &mouseHook, ::GetModuleHandleW(nullptr), 0);
    if (m_impl->state.hook == nullptr)
    {
        return false;
    }

    g_active = &m_impl->state;
    m_impl->state.active.store(true, std::memory_order_release);

    if (m_impl->state.window != nullptr)
    {
        ::ShowWindow(m_impl->state.window, SW_MINIMIZE);
    }

    return true;
}

void PointPicker::cancel()
{
    if (m_impl->state.hook != nullptr)
    {
        ::UnhookWindowsHookEx(m_impl->state.hook);
        m_impl->state.hook = nullptr;
    }

    if (g_active == &m_impl->state)
    {
        g_active = nullptr;
    }

    m_impl->state.active.store(false, std::memory_order_release);

    if (m_impl->state.window != nullptr)
    {
        ::ShowWindow(m_impl->state.window, SW_RESTORE);
        ::SetForegroundWindow(m_impl->state.window);
        m_impl->state.window = nullptr;
    }
}

bool PointPicker::isActive() const noexcept
{
    return m_impl->state.active.load(std::memory_order_acquire);
}

bool PointPicker::hasResult() const noexcept
{
    return m_impl->state.captured.load(std::memory_order_acquire);
}

ScreenPoint PointPicker::takeResult() noexcept
{
    const ScreenPoint point{.x = static_cast<std::int32_t>(m_impl->state.x.load(std::memory_order_relaxed)),
                            .y = static_cast<std::int32_t>(m_impl->state.y.load(std::memory_order_relaxed))};

    m_impl->state.captured.store(false, std::memory_order_release);
    return point;
}

ScreenPoint PointPicker::liveCursor() const noexcept
{
    return ScreenPoint{.x = static_cast<std::int32_t>(m_impl->state.liveX.load(std::memory_order_relaxed)),
                       .y = static_cast<std::int32_t>(m_impl->state.liveY.load(std::memory_order_relaxed))};
}

} // namespace deuca::platform
