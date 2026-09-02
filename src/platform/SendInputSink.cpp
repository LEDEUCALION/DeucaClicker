#include "platform/SendInputSink.hpp"

#include "platform/WindowsLean.hpp"

#include <algorithm>
#include <vector>

namespace deuca::platform
{
namespace
{

/// Lit l'étendue du bureau virtuel.
///
/// Relue à chaque lot comportant un déplacement plutôt que mise en cache à la
/// construction : brancher un écran change ces valeurs, et un puits qui garde
/// les anciennes viserait à côté sans rien signaler. L'appel est servi depuis
/// une zone partagée en mode utilisateur, son coût est négligeable devant
/// l'appel système qui suit.
[[nodiscard]] VirtualDesktop readVirtualDesktop() noexcept
{
    return VirtualDesktop{
        .origin = ScreenPoint{.x = ::GetSystemMetrics(SM_XVIRTUALSCREEN),
                              .y = ::GetSystemMetrics(SM_YVIRTUALSCREEN)},
        .width = std::max(1, ::GetSystemMetrics(SM_CXVIRTUALSCREEN)),
        .height = std::max(1, ::GetSystemMetrics(SM_CYVIRTUALSCREEN)),
    };
}

[[nodiscard]] DWORD buttonFlag(MouseButton button, ButtonAction action) noexcept
{
    const bool press = action == ButtonAction::Press;

    switch (button)
    {
    case MouseButton::Right:
        return press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    case MouseButton::Middle:
        return press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    case MouseButton::Left:
        break;
    }

    return press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
}

} // namespace

struct SendInputSink::Impl
{
    std::vector<INPUT> buffer;
    std::size_t maxBatchSize{0};
    VirtualDesktop desktop{};
    bool blocked{false};
};

SendInputSink::SendInputSink(std::size_t maxBatchSize) : m_impl{std::make_unique<Impl>()}
{
    m_impl->maxBatchSize = std::max<std::size_t>(2, maxBatchSize);

    // Alloué une fois pour toutes : la boucle de cadence ne doit rien allouer,
    // et surtout pas au moment précis où elle vise une échéance.
    m_impl->buffer.resize(m_impl->maxBatchSize);
}

SendInputSink::~SendInputSink() = default;

std::size_t SendInputSink::submit(std::span<const ClickEvent> events)
{
    m_impl->blocked = false;

    if (events.empty() || events.size() > m_impl->maxBatchSize)
    {
        return 0;
    }

    const bool needsDesktop =
        std::ranges::any_of(events, [](const ClickEvent& event) { return event.moveTo.has_value(); });
    if (needsDesktop)
    {
        m_impl->desktop = readVirtualDesktop();
    }

    std::size_t count = 0;
    for (const ClickEvent& event : events)
    {
        INPUT& input = m_impl->buffer[count++];
        input = INPUT{};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = buttonFlag(event.button, event.action);

        if (event.moveTo.has_value())
        {
            const ScreenPoint absolute = toAbsolute(*event.moveTo, m_impl->desktop);

            // VIRTUALDESK est indispensable dès qu'il y a plus d'un écran :
            // sans lui, l'échelle absolue ne couvre que l'écran principal et
            // tout ce qui est ailleurs devient inatteignable.
            input.mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
            input.mi.dx = absolute.x;
            input.mi.dy = absolute.y;
        }
    }

    const UINT sent = ::SendInput(static_cast<UINT>(count), m_impl->buffer.data(), sizeof(INPUT));

    // Un retour nul ne veut pas dire « rien à envoyer » : il veut dire que
    // l'entrée était déjà bloquée par un autre thread.
    m_impl->blocked = sent == 0 && count > 0;

    return sent;
}

std::size_t SendInputSink::maxBatchSize() const noexcept
{
    return m_impl->maxBatchSize;
}

bool SendInputSink::wasBlocked() const noexcept
{
    return m_impl->blocked;
}

VirtualDesktop SendInputSink::lastKnownDesktop() const noexcept
{
    return m_impl->desktop;
}

} // namespace deuca::platform
