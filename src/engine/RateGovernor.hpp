#pragma once

#include "core/GovernorPolicy.hpp"
#include "core/ITargetProbe.hpp"

#include <atomic>
#include <thread>

namespace deuca
{

/// Asservit la cadence à la réactivité de l'application visée.
///
/// Le sondage tourne sur son propre fil, et c'est le point central de cette
/// classe. Mesurer la réactivité d'une fenêtre demande de lui envoyer un
/// message et d'attendre sa réponse ; faire cela depuis le fil de cadence
/// interromprait les clics à chaque mesure, c'est-à-dire exactement l'inverse
/// du but poursuivi. Le gouverneur sonde de son côté et publie un facteur que
/// la boucle de clic lit sans jamais bloquer.
class RateGovernor
{
public:
    struct Config
    {
        GovernorConfig policy{};

        /// Intervalle entre deux sondages.
        ///
        /// Assez rapproché pour réagir avant que la file de la cible ne devienne
        /// ingérable, assez espacé pour que le sondage lui-même ne pèse pas sur
        /// ce qu'il mesure.
        Duration probeInterval{std::chrono::milliseconds{200}};
    };

    RateGovernor(ITargetProbe& probe, Config config = {});
    ~RateGovernor();

    RateGovernor(const RateGovernor&) = delete;
    RateGovernor& operator=(const RateGovernor&) = delete;
    RateGovernor(RateGovernor&&) = delete;
    RateGovernor& operator=(RateGovernor&&) = delete;

    /// Démarre l'asservissement. Le facteur repart de un.
    void start();

    /// Arrête l'asservissement et rend le facteur à un.
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    /// Facteur courant, entre le plancher et un.
    [[nodiscard]] double scale() const noexcept;

    /// Dernière latence mesurée, pour l'affichage.
    [[nodiscard]] Duration lastLatency() const noexcept;

    /// True si le dernier sondage a trouvé la cible figée.
    [[nodiscard]] bool targetHung() const noexcept;

    /// Nombre de sondages effectués depuis le démarrage.
    [[nodiscard]] std::uint64_t probeCount() const noexcept;

private:
    void run(std::stop_token token);

    ITargetProbe* m_probe;
    Config m_config;

    std::atomic<double> m_scale{1.0};
    std::atomic<Duration::rep> m_lastLatency{0};
    std::atomic<bool> m_targetHung{false};
    std::atomic<bool> m_running{false};
    std::atomic<std::uint64_t> m_probeCount{0};

    std::jthread m_worker;
};

} // namespace deuca
