#pragma once

#include "core/ClickPlan.hpp"
#include "engine/ClickEngine.hpp"
#include "engine/RateGovernor.hpp"
#include "platform/Hotkey.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace deuca::ui
{

/// Intervalle entre clics, tel que l'utilisateur le saisit.
///
/// Décomposé en heures, minutes, secondes et millisecondes plutôt qu'en
/// cadence : c'est la forme sous laquelle on pense une automatisation lente.
/// La cadence en est déduite, et l'interface affiche les deux.
struct IntervalFields
{
    int hours{0};
    int minutes{0};
    int seconds{0};
    int milliseconds{100};
};

/// Point d'assemblage de l'application.
///
/// Possède la minuterie, le puits, le moteur et le service de raccourcis, et
/// les fait tenir ensemble. Les panneaux ne parlent qu'à cet objet ; ils
/// n'ont jamais à connaître ni le moteur ni la plateforme.
///
/// L'ordre de déclaration des membres est significatif : le service de
/// raccourcis est déclaré en dernier, donc détruit en premier, pour qu'aucune
/// touche ne puisse déclencher une action sur un moteur déjà démantelé.
class AppController
{
public:
    AppController();
    ~AppController();

    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;
    AppController(AppController&&) = delete;
    AppController& operator=(AppController&&) = delete;

    /// Démarre si les conditions sont réunies, sinon renseigne le motif de
    /// refus consultable par l'interface.
    void start();

    /// Arrête la session en cours. Sans effet si rien ne tourne.
    void stop();

    /// Bascule marche/arrêt. C'est l'action du raccourci global.
    void toggle();

    [[nodiscard]] bool isRunning() const noexcept;

    /// Motif du dernier refus de démarrage.
    [[nodiscard]] StartRefusal lastRefusal() const noexcept;

    /// Motif du dernier refus, en une phrase destinée à l'utilisateur.
    [[nodiscard]] std::string refusalMessage() const;

    [[nodiscard]] EngineSnapshot snapshot() const noexcept;

    /// Asservissement de la cadence a la reactivite de la cible.
    void setGovernorEnabled(bool enabled) noexcept;
    [[nodiscard]] bool governorEnabled() const noexcept;

    /// Facteur courant applique a la cadence demandee, entre le plancher et un.
    [[nodiscard]] double governorScale() const noexcept;

    /// Derniere latence mesuree sur la fenetre visee.
    [[nodiscard]] Duration governorLatency() const noexcept;

    /// True si le dernier sondage a trouve la cible figee.
    [[nodiscard]] bool targetHung() const noexcept;

    /// Le plan en cours d'édition.
    ///
    /// Les modifications ne prennent effet qu'au démarrage suivant : changer la
    /// cadence d'une session en cours demanderait de recalculer les échéances
    /// en vol, ce qui produirait une rafale au moment du changement.
    [[nodiscard]] ClickPlan& plan() noexcept;
    [[nodiscard]] const ClickPlan& plan() const noexcept;

    [[nodiscard]] IntervalFields& interval() noexcept;

    /// Reporte l'intervalle saisi sur la cadence du plan.
    void applyInterval();

    /// Intervalle réellement appliqué, une fois la cadence plafonnée.
    [[nodiscard]] Duration effectiveInterval() const noexcept;

    [[nodiscard]] platform::Hotkey panicHotkey() const noexcept;
    [[nodiscard]] bool panicHotkeyActive() const noexcept;

    /// Change la combinaison d'arrêt d'urgence.
    ///
    /// @return false si le système l'a refusée ; l'ancienne reste active.
    bool rebindPanicHotkey(platform::Hotkey hotkey);

    /// Utiliser les points enregistrés plutôt que la position courante.
    ///
    /// Les points restent mémorisés quand le mode repasse sur la position
    /// courante : basculer d'un mode à l'autre ne doit pas effacer un travail
    /// de saisie.
    void setUseTargets(bool enabled) noexcept;
    [[nodiscard]] bool useTargets() const noexcept;

    /// Fenêtre principale, à effacer pendant une capture de point.
    void setMainWindow(void* window) noexcept;

    /// Démarre la désignation d'un point : l'application s'efface et le
    /// prochain clic de l'utilisateur, où qu'il soit, devient une cible.
    ///
    ///  false si le système a refusé l'interception.
    bool beginPointCapture();

    /// Interrompt une désignation en cours.
    void cancelPointCapture();

    [[nodiscard]] bool capturingPoint() const noexcept;

    /// Position du curseur pendant une désignation, pour l'affichage.
    [[nodiscard]] ScreenPoint liveCursor() const noexcept;

    /// À appeler une fois par image : récupère le point désigné s'il y en a un.
    ///
    /// Le résultat arrive depuis un crochet système, hors du flux de
    /// l'interface ; il faut donc venir le chercher.
    void pollPointCapture();

    /// Ajoute immédiatement la position courante du curseur à la liste.
    void captureTarget();

    /// Retire une cible par son rang.
    ///
    /// Sans effet si le rang est hors de la liste : une suppression sur une
    /// liste qui vient de changer sous les doigts ne doit pas planter.
    void removeTarget(std::size_t index);

    void clearTargets();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace deuca::ui
