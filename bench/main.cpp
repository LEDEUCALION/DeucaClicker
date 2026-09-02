#include "core/PrecisionWaiter.hpp"
#include "core/Statistics.hpp"
#include "core/Version.hpp"
#include "platform/CpuTopology.hpp"
#include "platform/HighResolutionTimer.hpp"
#include "platform/ThreadTuning.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>

//
// Mesure le retard au réveil du PrecisionWaiter sur plusieurs cadences, sans
// puis avec les réglages d'ordonnancement.
//
// Les deux passes d'une même cadence sont enchaînées, jamais menées en
// campagnes séparées : sinon la comparaison serait attaquable sur la charge de
// la machine entre les deux mesures.
//
// Les chaînes affichées sont volontairement sans accent : la console Windows
// les rendrait mal, et un rapport de mesure illisible ne vaut pas mieux que
// pas de rapport.
//

namespace
{
using namespace std::chrono_literals;

/// Cadences balayées, en hertz.
constexpr std::array kRatesHz{10, 100, 500, 1000, 2000};

/// Nombre minimum d'échantillons.
///
/// Deux cents et pas vingt : un p99 tiré de vingt mesures n'est pas un
/// percentile, c'est le maximum déguisé — le rang le plus proche y désigne le
/// dernier élément. En dessous d'une centaine d'échantillons, la queue de
/// distribution ne veut rien dire, et publier un chiffre qui ne veut rien dire
/// est pire que ne rien publier.
///
/// Le coût est supporté par les cadences lentes : vingt secondes à 10 Hz, et
/// deux fois plus depuis que chaque cadence est mesurée deux fois.
constexpr std::size_t kMinimumSamples = 200;

/// Tours non mesurés en tête de série, le temps que les caches et le timer se
/// mettent en régime.
constexpr std::size_t kWarmupIterations = 5;

[[nodiscard]] double toMicroseconds(deuca::Duration duration) noexcept
{
    return std::chrono::duration<double, std::micro>(duration).count();
}

[[nodiscard]] deuca::Duration periodFor(int rateHz) noexcept
{
    return std::chrono::duration_cast<deuca::Duration>(1s) / rateHz;
}

/// Mesure le retard au réveil pour une cadence donnée.
[[nodiscard]] std::vector<deuca::Duration> measure(deuca::PrecisionWaiter& waiter, int rateHz)
{
    const deuca::Duration period = periodFor(rateHz);
    const auto planned = std::max<std::size_t>(kMinimumSamples, static_cast<std::size_t>(rateHz));

    for (std::size_t i = 0; i < kWarmupIterations; ++i)
    {
        waiter.waitUntil(deuca::Clock::now() + period);
    }

    std::vector<deuca::Duration> lateness;
    lateness.reserve(planned);

    // Échéances calculées depuis un instant de départ fixe, jamais cumulées
    // d'un tour sur l'autre. Cumuler ferait reporter le retard de chaque tour
    // sur le suivant : on mesurerait une dérive au lieu d'une gigue.
    const deuca::Timestamp start = deuca::Clock::now();
    for (std::size_t i = 1; i <= planned; ++i)
    {
        const deuca::Timestamp deadline = start + period * static_cast<deuca::Duration::rep>(i);
        lateness.push_back(waiter.waitUntil(deadline) - deadline);
    }

    return lateness;
}

void printEnvironment(const deuca::platform::HighResolutionTimer& timer, const deuca::PrecisionWaiter& waiter)
{
    std::cout << std::format("{} -- banc de mesure d'attente\n\n", deuca::buildBanner());
    std::cout << std::format("Build                  : {}\n", DEUCA_BUILD_TYPE);
    std::cout << std::format("Timer                  : {}\n",
                             timer.isHighResolution() ? "haute resolution" : "tick systeme");
    std::cout << std::format("Granularite du dormeur : {:.1f} us\n", toMicroseconds(timer.granularity()));
    std::cout << std::format("Marge d'attente active : {:.1f} us\n", toMicroseconds(waiter.spinBudget()));

    const std::vector<deuca::platform::CpuCore> cores = deuca::platform::enumerateCores();
    std::cout << std::format("Coeurs logiques        : {} ({})\n", cores.size(),
                             deuca::platform::isHomogeneous(cores) ? "homogenes" : "hybrides");

    // Sonde jetable : on veut savoir quels leviers le systeme accepte
    // reellement avant d'interpreter les chiffres. Le reglage est defait des
    // la sortie du bloc.
    {
        const deuca::platform::ScopedThreadTuning probe;
        std::cout << std::format("Leviers acceptes       : MMCSS={} priorite={} coeurs={} energie={}\n",
                                 probe.mmcssActive(), probe.priorityActive(), probe.affinityActive(),
                                 probe.powerThrottlingDisabled());
    }

    if (std::string_view{DEUCA_BUILD_TYPE} == "Debug")
    {
        std::cout << "\nATTENTION : build Debug. Ces chiffres ne sont pas publiables.\n";
    }

    std::cout << std::format("\n{:>9} {:>8} {:>8} {:>9} {:>9} {:>9} {:>9} {:>9} {:>9}\n", "Cadence",
                             "Reglage", "Echant.", "min", "p50", "p90", "p99", "max", "moyenne");
    std::cout << std::format("{:->9} {:->8} {:->8} {:->9} {:->9} {:->9} {:->9} {:->9} {:->9}\n", "", "", "",
                             "", "", "", "", "", "");
}

void printRow(int rateHz, std::string_view label, const deuca::Summary& summary)
{
    std::cout << std::format(
        "{:>6} Hz {:>8} {:>8} {:>8.1f} {:>8.1f} {:>8.1f} {:>8.1f} {:>8.1f} {:>8.1f}\n", rateHz, label,
        summary.count, toMicroseconds(summary.min), toMicroseconds(summary.p50), toMicroseconds(summary.p90),
        toMicroseconds(summary.p99), toMicroseconds(summary.max), toMicroseconds(summary.mean));
}

} // namespace

int main()
{
    deuca::platform::HighResolutionTimer timer;
    if (!timer.isValid())
    {
        std::cerr << "Aucun timer attendable disponible : mesure impossible.\n";
        return 1;
    }

    deuca::PrecisionWaiter waiter{timer};
    printEnvironment(timer, waiter);

    for (const int rateHz : kRatesHz)
    {
        const std::vector<deuca::Duration> raw = measure(waiter, rateHz);

        std::vector<deuca::Duration> tuned;
        {
            const deuca::platform::ScopedThreadTuning tuning;
            tuned = measure(waiter, rateHz);
        }

        // Affichage hors du bloc regle : ecrire sur la sortie standard depuis
        // un thread a priorite critique n'a aucun interet et fausserait la
        // mesure suivante.
        printRow(rateHz, "brut", deuca::summarize(raw));
        printRow(rateHz, "regle", deuca::summarize(tuned));
    }

    std::cout << "\nRetard mesure entre l'echeance demandee et le reveil reel.\n"
                 "Percentiles par rang le plus proche, sans interpolation.\n"
                 "brut  = aucun reglage d'ordonnancement.\n"
                 "regle = MMCSS Pro Audio, priorite critique, coeurs performants,\n"
                 "        sortie du bridage energetique.\n";

    return 0;
}
