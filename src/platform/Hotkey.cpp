#include "platform/Hotkey.hpp"

#include "platform/WindowsLean.hpp"

namespace deuca::platform
{

Hotkey defaultPanicHotkey() noexcept
{
    return Hotkey{.modifiers = Modifier::None, .virtualKey = VK_F8};
}

bool HotkeyTable::add(int id, Callback callback)
{
    if (!callback)
    {
        return false;
    }

    return m_callbacks.try_emplace(id, std::move(callback)).second;
}

bool HotkeyTable::remove(int id)
{
    return m_callbacks.erase(id) > 0;
}

bool HotkeyTable::dispatch(int id) const
{
    const auto entry = m_callbacks.find(id);
    if (entry == m_callbacks.end())
    {
        return false;
    }

    entry->second();
    return true;
}

bool HotkeyTable::contains(int id) const noexcept
{
    return m_callbacks.contains(id);
}

std::size_t HotkeyTable::size() const noexcept
{
    return m_callbacks.size();
}

} // namespace deuca::platform
