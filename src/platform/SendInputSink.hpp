#pragma once

#include "core/IInputSink.hpp"
#include "core/ScreenCoordinates.hpp"

#include <cstddef>
#include <memory>

namespace deuca::platform
{

/// Puits d'injection reposant sur le flux d'entrée natif de Windows.
///
/// Le lot est soumis en un seul appel, et c'est tout l'intérêt : la
/// documentation garantit que les événements d'un même appel sont insérés
/// sérialisés et **ne sont pas entrelacés** avec les autres entrées, ni celles
/// de l'utilisateur ni celles d'un autre appel. Un lot de soixante-quatre clics
/// arrive donc propre, sans qu'un mouvement de souris réel vienne se glisser au
/// milieu d'un couple appui-relâchement.
///
/// Deux limites héritées du système, qu'il faut connaître avant d'interpréter
/// un échec. Un retour nul signifie que l'entrée était déjà bloquée par un
/// autre processus. Et l'injection est soumise à l'isolation des privilèges
/// d'interface : on ne peut atteindre qu'une application de niveau d'intégrité
/// égal ou inférieur, sans que ni le code de retour ni le code d'erreur ne le
/// signalent.
///
/// Le tampon d'événements vit derrière un pimpl, pour qu'il soit alloué une
/// fois pour toutes sans imposer <Windows.h> à qui inclut cet en-tête.
class SendInputSink final : public IInputSink
{
public:
    /// Plafond de sécurité par défaut, en événements.
    ///
    /// Ce n'est pas une limite du système : c'est un garde-fou. Un lot de
    /// plusieurs milliers de clics submerge la file de messages de
    /// l'application visée bien avant de gêner la nôtre.
    static constexpr std::size_t kDefaultMaxBatchSize = 512;

    explicit SendInputSink(std::size_t maxBatchSize = kDefaultMaxBatchSize);
    ~SendInputSink() override;

    std::size_t submit(std::span<const ClickEvent> events) override;

    [[nodiscard]] std::size_t maxBatchSize() const noexcept override;

    /// True si la dernière soumission a été refusée en bloc, ce qui veut dire
    /// que l'entrée est bloquée par un autre processus.
    [[nodiscard]] bool wasBlocked() const noexcept;

    /// Étendue du bureau virtuel telle que lue au dernier lot comportant un
    /// déplacement.
    [[nodiscard]] VirtualDesktop lastKnownDesktop() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace deuca::platform
