#pragma once

#include "platform/CpuTopology.hpp"

#include <memory>

namespace deuca::platform
{

/// Priorité de thread, exprimée sans exposer les constantes Win32.
///
/// La classe temps réel n'est volontairement pas représentable ici. Un thread
/// à priorité 31 qui tourne en attente active fige le bureau sur une machine à
/// faible nombre de cœurs, et aucune case à cocher ne rend cela acceptable.
enum class ThreadPriority
{
    Normal,
    AboveNormal,
    Highest,
    TimeCritical,
};

/// Priorité courante du thread appelant.
[[nodiscard]] ThreadPriority currentThreadPriority() noexcept;

/// Élève la priorité du thread appelant et la restaure à la destruction.
///
/// Seul le thread est touché, jamais la classe de priorité du processus :
/// TIME_CRITICAL vaut 15 aussi bien en classe normale qu'en classe haute, si
/// bien qu'élever la classe n'apporterait rien au thread de cadence tout en
/// accélérant tous les autres threads du processus sans raison.
class ScopedThreadPriority
{
public:
    explicit ScopedThreadPriority(ThreadPriority priority) noexcept;
    ~ScopedThreadPriority();

    ScopedThreadPriority(const ScopedThreadPriority&) = delete;
    ScopedThreadPriority& operator=(const ScopedThreadPriority&) = delete;
    ScopedThreadPriority(ScopedThreadPriority&&) = delete;
    ScopedThreadPriority& operator=(ScopedThreadPriority&&) = delete;

    [[nodiscard]] bool isActive() const noexcept;

private:
    ThreadPriority m_previous{ThreadPriority::Normal};
    bool m_active{false};
};

/// Inscrit le thread appelant dans la classe multimédia « Pro Audio ».
///
/// Préférable à TIME_CRITICAL seul, et c'est la nuance qui compte : MMCSS
/// accorde une priorité élevée **mais régulée**. Le service surveille la charge
/// et rétrograde le thread s'il dépasse son quota, avec un pourcentage de temps
/// processeur garanti aux tâches de moindre priorité. On obtient la réactivité
/// sans pouvoir affamer le système — exactement ce que font les moteurs audio
/// professionnels.
class ScopedMmcssTask
{
public:
    ScopedMmcssTask() noexcept;
    ~ScopedMmcssTask();

    ScopedMmcssTask(const ScopedMmcssTask&) = delete;
    ScopedMmcssTask& operator=(const ScopedMmcssTask&) = delete;
    ScopedMmcssTask(ScopedMmcssTask&&) = delete;
    ScopedMmcssTask& operator=(ScopedMmcssTask&&) = delete;

    /// False si le service MMCSS a refusé l'inscription, ce qui arrive
    /// notamment quand il est désactivé sur le poste.
    [[nodiscard]] bool isActive() const noexcept;

private:
    void* m_task{nullptr};
};

/// Sort le processus des politiques d'économie d'énergie, et rétablit le
/// comportement par défaut à la destruction.
///
/// Deux effets distincts derrière le même appel. EcoQoS autorise
/// l'ordonnanceur à nous parquer sur un cœur lent. Et depuis Windows 11, un
/// processus dont la fenêtre est réduite ou entièrement occultée perd la haute
/// résolution de ses timers — ce qui est précisément l'état dans lequel un
/// autoclicker passe sa vie.
class ScopedPowerThrottlingOptOut
{
public:
    ScopedPowerThrottlingOptOut() noexcept;
    ~ScopedPowerThrottlingOptOut();

    ScopedPowerThrottlingOptOut(const ScopedPowerThrottlingOptOut&) = delete;
    ScopedPowerThrottlingOptOut& operator=(const ScopedPowerThrottlingOptOut&) = delete;
    ScopedPowerThrottlingOptOut(ScopedPowerThrottlingOptOut&&) = delete;
    ScopedPowerThrottlingOptOut& operator=(ScopedPowerThrottlingOptOut&&) = delete;

    [[nodiscard]] bool isActive() const noexcept;

private:
    bool m_active{false};
};

/// Ce qu'on applique au thread de cadence.
struct TuningOptions
{
    bool useMmcss{true};
    bool raisePriority{true};
    bool pinToPerformanceCores{true};
    bool disablePowerThrottling{true};

    ThreadPriority priority{ThreadPriority::TimeCritical};
};

/// Applique l'ensemble des réglages retenus, et défait tout à la destruction.
///
/// Regroupés pour que le banc puisse mesurer une cadence avec puis sans, dans
/// la même exécution : deux campagnes séparées seraient attaquables sur la
/// charge de la machine entre les deux.
class ScopedThreadTuning
{
public:
    explicit ScopedThreadTuning(TuningOptions options = {});
    ~ScopedThreadTuning();

    ScopedThreadTuning(const ScopedThreadTuning&) = delete;
    ScopedThreadTuning& operator=(const ScopedThreadTuning&) = delete;
    ScopedThreadTuning(ScopedThreadTuning&&) = delete;
    ScopedThreadTuning& operator=(ScopedThreadTuning&&) = delete;

    [[nodiscard]] bool mmcssActive() const noexcept;
    [[nodiscard]] bool priorityActive() const noexcept;
    [[nodiscard]] bool affinityActive() const noexcept;
    [[nodiscard]] bool powerThrottlingDisabled() const noexcept;

private:
    // Déclarés dans l'ordre où le constructeur les applique, pour que la
    // destruction les défasse dans l'ordre inverse exact.
    std::unique_ptr<ScopedPowerThrottlingOptOut> m_power;
    std::unique_ptr<ScopedPerformanceCoreAffinity> m_affinity;
    std::unique_ptr<ScopedMmcssTask> m_mmcss;
    std::unique_ptr<ScopedThreadPriority> m_priority;
};

} // namespace deuca::platform
