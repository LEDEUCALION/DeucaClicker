#pragma once

#include "core/ClickPlan.hpp"
#include "core/IBlockingSleeper.hpp"
#include "core/IInputSink.hpp"
#include "core/Waiting.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

namespace deuca
{

/// Garde-fous et réglages du moteur.
struct EngineConfig
{
    /// Plafond dur de cadence. Un plan qui demande davantage est ramené ici.
    ///
    /// Ce n'est pas une limite technique mais une limite de responsabilité :
    /// au-delà, on ne fait plus que noyer la file de messages de la cible.
    double maxClicksPerSecond{20000.0};

    /// Durée maximale d'une session ; zéro lève la limite.
    ///
    /// Un homme mort logiciel. Un autoclicker oublié en marche est le mode de
    /// défaillance le plus courant du genre, et il ne se remarque qu'une fois
    /// les dégâts faits.
    Duration maxRunDuration{std::chrono::minutes{10}};

    /// Marge d'attente active transmise au waiter.
    Duration spinBudget{std::chrono::microseconds{300}};
};

/// Ramène un plan dans les limites du moteur et du puits.
///
/// Fonction libre et sans état : c'est de l'arithmétique de garde-fou, et elle
/// se vérifie sans démarrer quoi que ce soit.
[[nodiscard]] ClickPlan clampPlan(ClickPlan plan, const EngineConfig& config,
                                  std::size_t maxBatchSize) noexcept;

/// Pourquoi un démarrage est refusé.
enum class StartRefusal
{
    None,
    /// Aucun raccourci d'arrêt d'urgence n'est en place.
    NoPanicHotkey,
    /// Cadence nulle, négative, ou intervalle inexploitable.
    InvalidRate,
    /// Une session tourne déjà.
    AlreadyRunning,
};

/// Décide si une session peut démarrer.
///
/// Fonction libre et sans état : la règle de sûreté la plus importante du
/// projet ne doit pas dépendre de l'état d'une interface graphique pour être
/// vérifiable.
///
/// Le refus en l'absence de raccourci d'arrêt d'urgence n'est pas une
/// précaution excessive. Un autoclicker lancé sans moyen de l'arrêter au
/// clavier ne se rattrape qu'en atteignant son bouton d'arrêt à la souris —
/// avec une souris qui clique toute seule plusieurs centaines de fois par
/// seconde.
[[nodiscard]] StartRefusal evaluateStart(bool panicHotkeyActive, const ClickPlan& plan,
                                         bool alreadyRunning) noexcept;

/// Compteurs publiés par le moteur.
///
/// Lus champ par champ, sans verrou : ce n'est **pas** un instantané cohérent,
/// deux compteurs peuvent provenir de deux instants différents. C'est
/// acceptable pour de la télémétrie affichée à l'écran, et ce serait
/// inacceptable pour piloter une décision.
struct EngineSnapshot
{
    std::uint64_t burstsSubmitted{};
    std::uint64_t clicksEmitted{};
    std::uint64_t eventsRejected{};
    std::uint64_t blockedSubmissions{};
    bool running{};
};

/// Préparation appliquée au fil d'exécution au moment où il démarre.
///
/// Le jeton renvoyé est conservé pendant toute la vie du fil et détruit à son
/// arrêt. C'est ce qui permet à des réglages RAII — priorité, MMCSS, choix de
/// cœur — d'être posés **sur le fil lui-même** sans que cette couche ait à
/// connaître leur type, ni à inclure quoi que ce soit de la plateforme.
using ThreadPreparation = std::function<std::shared_ptr<void>()>;

/// La boucle de cadence.
///
/// Ne connaît ni Windows, ni l'interface graphique : elle consomme un puits et
/// un dormeur, tous deux abstraits. C'est ce qui la rend vérifiable avec des
/// doublures, fil d'exécution compris, sans qu'un test ne clique nulle part.
class ClickEngine
{
public:
    ClickEngine(IInputSink& sink, IBlockingSleeper& sleeper, EngineConfig config = {});
    ~ClickEngine();

    ClickEngine(const ClickEngine&) = delete;
    ClickEngine& operator=(const ClickEngine&) = delete;
    ClickEngine(ClickEngine&&) = delete;
    ClickEngine& operator=(ClickEngine&&) = delete;

    /// Installe la préparation du fil d'exécution. Sans effet sur une session
    /// déjà démarrée.
    void setThreadPreparation(ThreadPreparation preparation);

    /// Démarre une session. Un appel sur un moteur déjà en marche l'arrête
    /// d'abord, pour qu'il n'existe jamais deux boucles concurrentes.
    void start(ClickPlan plan);

    /// Demande l'arrêt et attend la fin du fil d'exécution.
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    [[nodiscard]] EngineSnapshot snapshot() const noexcept;

private:
    void run(std::stop_token token, ClickPlan plan);

    IInputSink* m_sink;
    IBlockingSleeper* m_sleeper;
    EngineConfig m_config;
    ThreadPreparation m_preparation;

    std::atomic<std::uint64_t> m_burstsSubmitted{0};
    std::atomic<std::uint64_t> m_clicksEmitted{0};
    std::atomic<std::uint64_t> m_eventsRejected{0};
    std::atomic<std::uint64_t> m_blockedSubmissions{0};
    std::atomic<bool> m_running{false};

    std::jthread m_worker;
};

} // namespace deuca
