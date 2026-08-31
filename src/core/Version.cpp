#include "core/Version.hpp"

#include <format>

namespace deuca
{

std::string Version::toString() const
{
    return std::format("{}.{}.{}", major, minor, patch);
}

Version currentVersion() noexcept
{
    return Version{DEUCA_VERSION_MAJOR, DEUCA_VERSION_MINOR, DEUCA_VERSION_PATCH};
}

std::string buildBanner()
{
    return std::format("DeucaClicker {}", currentVersion().toString());
}

} // namespace deuca
