#pragma once

#include "core/ClickPlan.hpp"

#include <cstddef>
#include <span>

namespace deuca
{

/// Fabrique les événements d'un lot à partir d'un plan.
///
/// Séparé du moteur à dessein : c'est ici que se décide l'ordre exact des
/// appuis, des relâchements et des déplacements, et c'est exactement le genre
/// de logique qui se casse en silence. Isolée, elle se vérifie dans un test
/// unitaire sans thread, sans horloge et sans toucher au curseur.
///
/// L'état conservé d'un lot à l'autre est la cible courante : les points sont
/// parcourus en boucle et la rotation ne se réinitialise pas à chaque lot,
/// sans quoi un lot de deux clics sur trois cibles ne visiterait jamais la
/// troisième.
class ClickSequencer
{
public:
    explicit ClickSequencer(ClickPlan plan) noexcept;

    /// Remplit le tampon avec un lot complet.
    ///
    /// @return le nombre d'événements écrits, ou zéro si le tampon est trop
    ///         petit — un lot tronqué laisserait un bouton enfoncé.
    std::size_t fillBurst(std::span<ClickEvent> buffer) noexcept;

    /// Nombre d'événements qu'un lot occupera.
    [[nodiscard]] std::size_t eventsPerBurst() const noexcept;

    /// Remet la rotation des cibles à son point de départ.
    void reset() noexcept;

    [[nodiscard]] const ClickPlan& plan() const noexcept;

private:
    ClickPlan m_plan;
    std::size_t m_nextTarget{0};
};

} // namespace deuca
