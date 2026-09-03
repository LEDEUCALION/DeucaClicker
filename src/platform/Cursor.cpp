#include "platform/Cursor.hpp"

#include "platform/WindowsLean.hpp"

namespace deuca::platform
{

ScreenPoint currentCursorPosition() noexcept
{
    POINT point{};
    if (::GetCursorPos(&point) == FALSE)
    {
        return ScreenPoint{};
    }

    return ScreenPoint{.x = point.x, .y = point.y};
}

} // namespace deuca::platform
