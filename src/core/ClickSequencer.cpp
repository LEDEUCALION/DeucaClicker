#include "core/ClickSequencer.hpp"

#include <algorithm>

namespace deuca
{

ClickSequencer::ClickSequencer(ClickPlan plan) noexcept : m_plan{std::move(plan)} {}

std::size_t ClickSequencer::fillBurst(std::span<ClickEvent> buffer, std::size_t maxActivations) noexcept
{
    if (maxActivations == 0)
    {
        return 0;
    }

    const std::size_t activations = std::min(std::max<std::size_t>(1, m_plan.burstSize), maxActivations);
    const std::size_t presses = pressesPerActivation(m_plan.style);
    const std::size_t needed = activations * presses * 2;

    if (buffer.size() < needed)
    {
        // Refus net plutôt que remplissage partiel : un lot coupé entre l'appui
        // et le relâchement laisserait le bouton enfoncé, et l'utilisateur
        // découvrirait le problème en voyant sa sélection s'étendre à tout
        // l'écran.
        return 0;
    }

    std::size_t written = 0;
    for (std::size_t activation = 0; activation < activations; ++activation)
    {
        std::optional<ScreenPoint> target;
        if (!m_plan.targets.empty())
        {
            target = m_plan.targets[m_nextTarget];
            m_nextTarget = (m_nextTarget + 1) % m_plan.targets.size();
        }

        // La cible avance une fois par activation, pas une fois par appui : un
        // double-clic doit tomber deux fois au même endroit, sinon ce n'est
        // plus un double-clic.
        for (std::size_t press = 0; press < presses; ++press)
        {
            ClickEvent down{};
            down.button = m_plan.button;
            down.action = ButtonAction::Press;
            down.moveTo = target;
            buffer[written++] = down;

            // Le relâchement ne se déplace jamais : bouger entre l'appui et le
            // relâchement produirait un glisser, pas un clic.
            ClickEvent up{};
            up.button = m_plan.button;
            up.action = ButtonAction::Release;
            buffer[written++] = up;
        }
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
