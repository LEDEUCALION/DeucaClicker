#pragma once

#include "core/IBlockingSleeper.hpp"

namespace deuca::platform
{

/// Dormeur bloquant fondé sur un objet timer attendable haute résolution.
///
/// C'est la primitive d'attente moderne de Windows, et elle est préférable aux
/// deux réflexes habituels. timeBeginPeriod agit sur tout le processus, coûte
/// en consommation, et depuis Windows 11 sa haute résolution est retirée aux
/// fenêtres masquées ou réduites — exactement notre cas d'usage. Quant à
/// NtSetTimerResolution, il n'est pas documenté et se mesure moins régulier
/// sous charge. Ici la résolution est portée par l'objet lui-même, sans effet
/// de bord sur le reste du système.
class HighResolutionTimer final : public IBlockingSleeper
{
public:
    HighResolutionTimer() noexcept;
    ~HighResolutionTimer() override;

    [[nodiscard]] Duration granularity() const noexcept override;

    void sleepFor(Duration duration) override;

    /// False si le système a refusé le drapeau haute résolution et qu'on est
    /// retombé sur un timer ordinaire. L'attente reste correcte, simplement
    /// plus grossière : la marge d'attente active se recale dessus.
    [[nodiscard]] bool isHighResolution() const noexcept;

    /// False si aucun timer n'a pu être créé. sleepFor devient alors inopérant
    /// et l'attente se joue entièrement en actif, ce qu'il faut savoir avant de
    /// lancer une cadence longue.
    [[nodiscard]] bool isValid() const noexcept;

private:
    // HANDLE, gardé en void* pour ne pas imposer <Windows.h> à qui inclut cet
    // en-tête.
    void* m_timer{nullptr};
    bool m_highResolution{false};
};

} // namespace deuca::platform
