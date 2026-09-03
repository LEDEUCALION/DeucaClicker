#pragma once

#include "core/ITargetProbe.hpp"

namespace deuca::platform
{

/// Sonde la fenêtre au premier plan, c'est-à-dire celle qui reçoit nos clics.
///
/// La méthode tient en une phrase : on envoie un message vide et on mesure le
/// temps qu'il met à revenir. Le message ne fait rien, donc tout le temps
/// mesuré est du temps passé dans la file de la cible. C'est le seul moyen
/// propre d'observer cet engorgement — la file d'un autre processus n'est pas
/// exposée, GetQueueStatus ne renseigne que la nôtre.
///
/// Le sondage exclut délibérément notre propre fenêtre. Se mesurer soi-même
/// donnerait une latence toujours saine et un asservissement qui ne freine
/// jamais.
class ForegroundProbe final : public ITargetProbe
{
public:
    /// @param timeout délai au-delà duquel la cible est déclarée figée.
    explicit ForegroundProbe(Duration timeout = std::chrono::milliseconds{200}) noexcept;

    [[nodiscard]] ProbeResult probe() override;

private:
    Duration m_timeout;
};

} // namespace deuca::platform
