#include "platform/CpuTopology.hpp"

#include "platform/WindowsLean.hpp"

#include <algorithm>

namespace deuca::platform
{
namespace
{

/// Récupère la description des jeux de processeurs dans un tampon.
[[nodiscard]] std::vector<std::uint8_t> queryCpuSetInformation()
{
    ULONG required = 0;
    ::GetSystemCpuSetInformation(nullptr, 0, &required, ::GetCurrentProcess(), 0);
    if (required == 0)
    {
        return {};
    }

    std::vector<std::uint8_t> buffer(required);
    if (::GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data()), required,
                                     &required, ::GetCurrentProcess(), 0) == FALSE)
    {
        return {};
    }

    return buffer;
}

} // namespace

std::vector<CpuCore> enumerateCores()
{
    const std::vector<std::uint8_t> buffer = queryCpuSetInformation();

    std::vector<CpuCore> cores;
    std::size_t offset = 0;
    while (offset + sizeof(SYSTEM_CPU_SET_INFORMATION) <= buffer.size())
    {
        const auto* entry = reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION*>(buffer.data() + offset);
        if (entry->Size == 0)
        {
            break;
        }

        if (entry->Type == CpuSetInformation)
        {
            cores.push_back(
                CpuCore{.id = entry->CpuSet.Id, .efficiencyClass = entry->CpuSet.EfficiencyClass});
        }

        offset += entry->Size;
    }

    return cores;
}

bool isHomogeneous(const std::vector<CpuCore>& cores) noexcept
{
    if (cores.empty())
    {
        return true;
    }

    const std::uint8_t reference = cores.front().efficiencyClass;
    return std::ranges::all_of(
        cores, [reference](const CpuCore& core) { return core.efficiencyClass == reference; });
}

ScopedPerformanceCoreAffinity::ScopedPerformanceCoreAffinity() noexcept
{
    const std::vector<CpuCore> cores = enumerateCores();
    if (cores.empty())
    {
        return;
    }

    const auto best = std::ranges::max_element(cores, {}, &CpuCore::efficiencyClass);

    std::vector<ULONG> selected;
    selected.reserve(cores.size());
    for (const CpuCore& core : cores)
    {
        if (core.efficiencyClass == best->efficiencyClass)
        {
            selected.push_back(core.id);
        }
    }

    if (::SetThreadSelectedCpuSets(::GetCurrentThread(), selected.data(),
                                   static_cast<ULONG>(selected.size())) != FALSE)
    {
        m_active = true;
        m_selectedCount = selected.size();
    }
}

ScopedPerformanceCoreAffinity::~ScopedPerformanceCoreAffinity()
{
    if (m_active)
    {
        // Un tableau vide lève la sélection et rend le thread à l'ordonnanceur.
        ::SetThreadSelectedCpuSets(::GetCurrentThread(), nullptr, 0);
    }
}

bool ScopedPerformanceCoreAffinity::isActive() const noexcept
{
    return m_active;
}

std::size_t ScopedPerformanceCoreAffinity::selectedCount() const noexcept
{
    return m_selectedCount;
}

} // namespace deuca::platform
