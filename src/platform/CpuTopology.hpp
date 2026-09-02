#pragma once

#include <cstdint>
#include <vector>

namespace deuca::platform
{

/// Un cœur logique tel que Windows le décrit.
struct CpuCore
{
    /// Identifiant de jeu de processeurs, tel qu'attendu par les API CpuSet.
    std::uint32_t id{};

    /// Classe d'efficacité : plus la valeur est grande, plus le cœur est
    /// performant. Sur un processeur homogène, tous les cœurs partagent la
    /// même valeur.
    std::uint8_t efficiencyClass{};
};

/// Énumère les cœurs logiques visibles par le processus.
///
/// Renvoie une liste vide si le système refuse de la fournir, ce qui doit être
/// traité comme « pas d'information » et non comme « pas de cœurs ».
[[nodiscard]] std::vector<CpuCore> enumerateCores();

/// True si tous les cœurs partagent la même classe d'efficacité.
[[nodiscard]] bool isHomogeneous(const std::vector<CpuCore>& cores) noexcept;

/// Restreint le thread appelant aux cœurs de la classe la plus performante,
/// et lève la restriction à la destruction.
///
/// Sans effet utile sur un processeur homogène : il n'y a alors qu'une classe,
/// et la sélection couvre tous les cœurs. L'intérêt est sur les architectures
/// hybrides, où l'ordonnanceur peut déplacer un thread d'attente sur un cœur
/// efficient et y perdre plusieurs centaines de microsecondes au réveil.
///
/// La sélection par jeu de processeurs est préférée à un masque d'affinité
/// figé : elle exprime une préférence, là où SetThreadAffinityMask interdit à
/// l'ordonnanceur de nous déplacer même quand le cœur choisi devient chaud.
class ScopedPerformanceCoreAffinity
{
public:
    ScopedPerformanceCoreAffinity() noexcept;
    ~ScopedPerformanceCoreAffinity();

    ScopedPerformanceCoreAffinity(const ScopedPerformanceCoreAffinity&) = delete;
    ScopedPerformanceCoreAffinity& operator=(const ScopedPerformanceCoreAffinity&) = delete;
    ScopedPerformanceCoreAffinity(ScopedPerformanceCoreAffinity&&) = delete;
    ScopedPerformanceCoreAffinity& operator=(ScopedPerformanceCoreAffinity&&) = delete;

    /// True si une sélection a effectivement été posée.
    [[nodiscard]] bool isActive() const noexcept;

    /// Nombre de cœurs retenus par la sélection.
    [[nodiscard]] std::size_t selectedCount() const noexcept;

private:
    bool m_active{false};
    std::size_t m_selectedCount{0};
};

} // namespace deuca::platform
