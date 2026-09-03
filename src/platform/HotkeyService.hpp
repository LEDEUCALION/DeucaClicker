#pragma once

#include "platform/Hotkey.hpp"

#include <memory>
#include <optional>

namespace deuca::platform
{

/// Écoute des raccourcis clavier globaux, sur son propre fil d'exécution.
///
/// Trois décisions de conception, et la première est la plus importante.
///
/// Le service vit sur un fil séparé, derrière une fenêtre sans affichage. Le
/// raccourci d'arrêt d'urgence continue donc de répondre même si l'interface
/// est occupée, figée ou en train d'être redessinée. Le brancher sur la fenêtre
/// principale reviendrait à faire dépendre l'arrêt d'urgence de la santé de ce
/// qu'il doit pouvoir arrêter.
///
/// Le raccourci de panique est enregistré à la construction et **aucune méthode
/// ne permet de le retirer**. Ce n'est pas une omission : un arrêt d'urgence
/// désactivable n'est pas un arrêt d'urgence.
///
/// Les actions sont exécutées sur le fil du service. Elles doivent être brèves
/// et ne rien bloquer : tant qu'une action tourne, aucun autre raccourci n'est
/// traité, y compris celui de panique.
class HotkeyService
{
public:
    /// Démarre le service et enregistre le raccourci d'arrêt d'urgence.
    ///
    /// L'action de panique est obligatoire : construire ce service sans elle
    /// n'aurait pas de sens.
    HotkeyService(Hotkey panicHotkey, HotkeyTable::Callback onPanic);
    ~HotkeyService();

    HotkeyService(const HotkeyService&) = delete;
    HotkeyService& operator=(const HotkeyService&) = delete;
    HotkeyService(HotkeyService&&) = delete;
    HotkeyService& operator=(HotkeyService&&) = delete;

    /// True si le fil d'écoute tourne et si sa fenêtre est en place.
    [[nodiscard]] bool isRunning() const noexcept;

    /// True si le système a bien accordé le raccourci de panique.
    ///
    /// False signifie qu'une autre application le détient déjà. L'appelant doit
    /// le signaler à l'utilisateur plutôt que de le taire : sans arrêt
    /// d'urgence, il ne faut rien lancer.
    [[nodiscard]] bool panicHotkeyActive() const noexcept;

    /// Change la combinaison de l'arrêt d'urgence.
    ///
    /// Reconfigurable n'est pas désactivable : l'opération est atomique du
    /// point de vue de l'utilisateur. L'ancienne combinaison est retirée, la
    /// nouvelle tentée, et si le système la refuse — une autre application la
    /// détient déjà — l'ancienne est remise en place. Il n'existe aucun instant
    /// observable où l'arrêt d'urgence serait absent.
    ///
    /// @return false si la nouvelle combinaison a été refusée. L'ancienne est
    ///         alors toujours active.
    bool rebindPanicHotkey(Hotkey hotkey);

    /// Combinaison actuellement affectée à l'arrêt d'urgence.
    [[nodiscard]] Hotkey panicHotkey() const noexcept;

    /// Enregistre un raccourci supplémentaire.
    ///
    /// Synchrone : l'enregistrement est effectué sur le fil du service, qui est
    /// le seul à pouvoir le faire, et le résultat est attendu.
    ///
    /// @return l'identifiant attribué, ou rien si le système a refusé — le plus
    ///         souvent parce qu'une autre application détient la combinaison.
    [[nodiscard]] std::optional<int> registerHotkey(Hotkey hotkey, HotkeyTable::Callback callback);

    /// Retire un raccourci précédemment enregistré.
    ///
    /// @return false si l'identifiant est inconnu, ou s'il s'agit de celui de
    ///         l'arrêt d'urgence.
    bool unregisterHotkey(int id);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace deuca::platform
