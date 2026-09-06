#include "platform/SystemFont.hpp"

#include "platform/WindowsLean.hpp"

#include <filesystem>

namespace deuca::platform
{
namespace
{

[[nodiscard]] const wchar_t* fileNameFor(FontWeight weight) noexcept
{
    switch (weight)
    {
    case FontWeight::SemiBold:
        return L"seguisb.ttf";
    case FontWeight::Bold:
        return L"segoeuib.ttf";
    case FontWeight::Regular:
        break;
    }

    return L"segoeui.ttf";
}

} // namespace

std::string segoeUiPath(FontWeight weight)
{
    // GetWindowsDirectory plutôt que le chemin codé en dur : l'installation
    // n'est pas toujours sur C, et une image d'entreprise peut la déplacer.
    wchar_t windows[MAX_PATH]{};
    const UINT length = ::GetWindowsDirectoryW(windows, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
        return {};
    }

    std::filesystem::path candidate{windows};
    candidate /= L"Fonts";
    candidate /= fileNameFor(weight);

    std::error_code error;
    if (!std::filesystem::exists(candidate, error))
    {
        return {};
    }

    // string() convertit en encodage natif étroit ; le chemin ne contient que
    // des caractères ASCII sur une installation standard.
    return candidate.string();
}

} // namespace deuca::platform
