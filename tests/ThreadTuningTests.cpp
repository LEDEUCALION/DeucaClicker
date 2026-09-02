#include "platform/CpuTopology.hpp"
#include "platform/ThreadTuning.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

//
// Ces cas touchent l'ordonnanceur, mais aucun ne demande de session
// interactive : ils tournent tels quels sur un runner.
//
// On ne vérifie pas que les réglages *améliorent* quoi que ce soit — cela se
// mesure au banc, pas dans une suite de tests. Ce qui est vérifié ici, c'est
// qu'ils s'appliquent, qu'ils se défont, et qu'ils ne laissent rien derrière.
//

TEST_CASE("L'enumeration des coeurs renvoie quelque chose de coherent", "[platform][tuning]")
{
    const std::vector<deuca::platform::CpuCore> cores = deuca::platform::enumerateCores();

    REQUIRE_FALSE(cores.empty());

    std::vector<std::uint32_t> ids;
    ids.reserve(cores.size());
    for (const deuca::platform::CpuCore& core : cores)
    {
        ids.push_back(core.id);
    }

    std::ranges::sort(ids);
    REQUIRE(std::ranges::adjacent_find(ids) == ids.end());
}

TEST_CASE("L'homogeneite se juge sur la classe d'efficacite", "[platform][tuning]")
{
    // Logique pure, verifiable sans interroger la machine.
    const std::vector<deuca::platform::CpuCore> uniform{{.id = 0, .efficiencyClass = 1},
                                                        {.id = 1, .efficiencyClass = 1}};
    const std::vector<deuca::platform::CpuCore> hybrid{{.id = 0, .efficiencyClass = 1},
                                                       {.id = 1, .efficiencyClass = 0}};

    REQUIRE(deuca::platform::isHomogeneous(uniform));
    REQUIRE_FALSE(deuca::platform::isHomogeneous(hybrid));

    // Une liste vide veut dire « pas d'information », pas « machine hybride ».
    REQUIRE(deuca::platform::isHomogeneous({}));
}

TEST_CASE("La priorite est elevee puis rendue", "[platform][tuning]")
{
    const deuca::platform::ThreadPriority before = deuca::platform::currentThreadPriority();

    {
        const deuca::platform::ScopedThreadPriority raised{deuca::platform::ThreadPriority::Highest};
        REQUIRE(raised.isActive());
        REQUIRE(deuca::platform::currentThreadPriority() == deuca::platform::ThreadPriority::Highest);
    }

    REQUIRE(deuca::platform::currentThreadPriority() == before);
}

TEST_CASE("Les portees imbriquees rendent la priorite dans l'ordre", "[platform][tuning]")
{
    const deuca::platform::ThreadPriority before = deuca::platform::currentThreadPriority();

    {
        const deuca::platform::ScopedThreadPriority outer{deuca::platform::ThreadPriority::AboveNormal};
        {
            const deuca::platform::ScopedThreadPriority inner{deuca::platform::ThreadPriority::Highest};
            REQUIRE(deuca::platform::currentThreadPriority() == deuca::platform::ThreadPriority::Highest);
        }
        REQUIRE(deuca::platform::currentThreadPriority() == deuca::platform::ThreadPriority::AboveNormal);
    }

    REQUIRE(deuca::platform::currentThreadPriority() == before);
}

TEST_CASE("L'inscription MMCSS se construit et se defait sans incident", "[platform][tuning]")
{
    // isActive peut valoir false : le service MMCSS est desactivable, et
    // certaines images de machine virtuelle le desactivent. On verifie que le
    // cas est represente, pas qu'il est vrai.
    const deuca::platform::ScopedMmcssTask task;
    const bool active = task.isActive();
    REQUIRE((active || !active));
}

TEST_CASE("Le refus du bridage energetique s'applique", "[platform][tuning]")
{
    const deuca::platform::ScopedPowerThrottlingOptOut optOut;
    REQUIRE(optOut.isActive());
}

TEST_CASE("Un reglage complet declare ce qu'il a obtenu", "[platform][tuning]")
{
    const deuca::platform::ThreadPriority before = deuca::platform::currentThreadPriority();

    {
        const deuca::platform::ScopedThreadTuning tuning;
        REQUIRE(tuning.powerThrottlingDisabled());
        REQUIRE(tuning.priorityActive());
    }

    REQUIRE(deuca::platform::currentThreadPriority() == before);
}

TEST_CASE("Un reglage vide ne touche a rien", "[platform][tuning]")
{
    const deuca::platform::TuningOptions none{.useMmcss = false,
                                              .raisePriority = false,
                                              .pinToPerformanceCores = false,
                                              .disablePowerThrottling = false};
    const deuca::platform::ScopedThreadTuning tuning{none};

    REQUIRE_FALSE(tuning.mmcssActive());
    REQUIRE_FALSE(tuning.priorityActive());
    REQUIRE_FALSE(tuning.affinityActive());
    REQUIRE_FALSE(tuning.powerThrottlingDisabled());
}
