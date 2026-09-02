#include "core/ClickSequencer.hpp"

#include <algorithm>

namespace deuca
{

ClickSequencer::ClickSequencer(ClickPlan plan) noexcept : m_plan{std::move(plan)} {}

std::size_t ClickSequencer::fillBurst(std::span<ClickEvent> buffer) noexcept
{
    const std::size_t needed = eventsPerBurst();
    if (buffer.size() < needed)
    {
        // Refus net plutôt que remplissage partiel : un lot coupé entre l'appui
        // et le relâchement laisserait le bouton enfoncé, et l'utilisateur
        // découvrirait le problème en voyant sa sélection s'étendre à tout
        // l'écran.
        return 0;
    }

    const std::size_t clicks = std::max<std::size_t>(1, m_plan.burstSize);

    std::size_t written = 0;
    for (std::size_t i = 0; i < clicks; ++i)
    {
        ClickEvent press{};
        press.button = m_plan.button;
        press.action = ButtonAction::Press;

        if (!m_plan.targets.empty())
        {
            press.moveTo = m_plan.targets[m_nextTarget];
            m_nextTarget = (m_nextTarget + 1) % m_plan.targets.size();
        }

        buffer[written++] = press;

        // Le relâchement ne se déplace jamais : bouger entre l'appui et le
        // relâchement produirait un glisser, pas un clic.
        ClickEvent release{};
        release.button = m_plan.button;
        release.action = ButtonAction::Release;

        buffer[written++] = release;
    }

    return written;
}

std::size_t ClickSequencer::eventsPerBurst() const noexcept
{
    return deuca::eventsPerBurst(m_plan);
}

void ClickSequencer::reset() noexcept
{
    m_nextTarget = 0;
}

const ClickPlan& ClickSequencer::plan() const noexcept
{
    return m_plan;
}

} // namespace deuca
