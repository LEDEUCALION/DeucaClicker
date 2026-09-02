#pragma once

#include "core/ClickEvent.hpp"

#include <cstddef>
#include <span>

namespace deuca
{

/// Destination d'un lot d'événements d'entrée.
///
/// Abstraite pour deux raisons. D'abord parce qu'il y aura plusieurs
/// implantations, dont un mode de compatibilité qui ne passe pas par le flux
/// global. Ensuite et surtout parce qu'un puits simulé rend l'assemblage des
/// lots vérifiable sans rien injecter dans le système : on peut prouver
/// l'ordre des événements et la taille des lots sans qu'un test fasse bouger
/// le curseur de la machine qui l'exécute.
class IInputSink
{
public:
    virtual ~IInputSink() = default;

    IInputSink(const IInputSink&) = delete;
    IInputSink& operator=(const IInputSink&) = delete;
    IInputSink(IInputSink&&) = delete;
    IInputSink& operator=(IInputSink&&) = delete;

    /// Soumet un lot au flux d'entrée.
    ///
    /// @return le nombre d'événements effectivement acceptés. Un retour
    ///         inférieur à la taille du lot doit être signalé et non ignoré :
    ///         il peut vouloir dire que l'entrée est bloquée par un autre
    ///         processus, ou que la cible refuse notre niveau d'intégrité.
    virtual std::size_t submit(std::span<const ClickEvent> events) = 0;

    /// Nombre maximal d'événements acceptés en un seul lot.
    [[nodiscard]] virtual std::size_t maxBatchSize() const noexcept = 0;

protected:
    IInputSink() = default;
};

} // namespace deuca
