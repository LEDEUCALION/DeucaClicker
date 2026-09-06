#pragma once

#include <imgui.h>

namespace deuca::ui
{

/// Ce qu'un bouton annonce avant d'être pressé.
///
/// La couleur porte l'intention, elle ne décore pas. Un utilisateur doit
/// pouvoir prévoir la conséquence d'un clic sans lire le libellé — c'est
/// surtout vrai des actions destructrices, qu'on ne veut jamais déclencher par
/// méprise.
enum class ButtonTone
{
    /// Action banale et réversible.
    Neutral,
    /// Action constructive : ajouter, désigner, valider.
    Accent,
    /// Action destructrice : vider, supprimer.
    Danger,
};

/// Bouton secondaire teinté selon son intention.
///
/// Teinté, jamais rempli d'un aplat plein : un seul objet de l'application
/// porte un aplat de couleur pleine, et c'est l'action principale. Si trois
/// boutons crient en même temps, aucun ne se distingue.
[[nodiscard]] bool toneButton(const char* label, ImVec2 size, ButtonTone tone, bool enabled = true);

/// Petit bouton de suppression, marqué en rouge dès le repos.
///
/// La croix est rouge avant même le survol. Attendre le survol pour signaler
/// qu'un bouton détruit quelque chose, c'est prévenir l'utilisateur au moment
/// où sa main est déjà partie.
///
/// @param id identifiant unique dans la pile ImGui courante.
[[nodiscard]] bool deleteButton(const char* id, float side);

} // namespace deuca::ui
