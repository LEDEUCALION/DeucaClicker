#include "core/Statistics.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>

using namespace std::chrono_literals;

TEST_CASE("Un echantillon vide donne un resume a zero", "[stats]")
{
    const deuca::Summary summary = deuca::summarize({});

    REQUIRE(summary.count == 0);
    REQUIRE(summary.min == deuca::Duration::zero());
    REQUIRE(summary.max == deuca::Duration::zero());
    REQUIRE(summary.mean == deuca::Duration::zero());
}

TEST_CASE("Un echantillon unique se resume a lui meme", "[stats]")
{
    const std::array samples{deuca::Duration{7us}};
    const deuca::Summary summary = deuca::summarize(samples);

    REQUIRE(summary.count == 1);
    REQUIRE(summary.min == deuca::Duration{7us});
    REQUIRE(summary.p50 == deuca::Duration{7us});
    REQUIRE(summary.p99 == deuca::Duration{7us});
    REQUIRE(summary.max == deuca::Duration{7us});
    REQUIRE(summary.mean == deuca::Duration{7us});
}

TEST_CASE("Le resume trie l'echantillon avant de conclure", "[stats]")
{
    // Volontairement dans le desordre : c'est l'ordre d'acquisition d'un banc.
    const std::array samples{deuca::Duration{5ns}, deuca::Duration{1ns}, deuca::Duration{3ns},
                             deuca::Duration{2ns}, deuca::Duration{4ns}};
    const deuca::Summary summary = deuca::summarize(samples);

    REQUIRE(summary.count == 5);
    REQUIRE(summary.min == deuca::Duration{1ns});
    REQUIRE(summary.max == deuca::Duration{5ns});
    REQUIRE(summary.mean == deuca::Duration{3ns});
}

TEST_CASE("Le percentile suit le rang le plus proche", "[stats]")
{
    // Rang le plus proche sur cinq echantillons tries 1..5 :
    //   p50 -> ceil(0,50 x 5) - 1 = 2 -> 3 ns
    //   p90 -> ceil(0,90 x 5) - 1 = 4 -> 5 ns
    const std::array sorted{deuca::Duration{1ns}, deuca::Duration{2ns}, deuca::Duration{3ns},
                            deuca::Duration{4ns}, deuca::Duration{5ns}};

    REQUIRE(deuca::percentile(sorted, 0.50) == deuca::Duration{3ns});
    REQUIRE(deuca::percentile(sorted, 0.90) == deuca::Duration{5ns});
    REQUIRE(deuca::percentile(sorted, 0.99) == deuca::Duration{5ns});
}

TEST_CASE("Le percentile borne les fractions hors intervalle", "[stats]")
{
    const std::array sorted{deuca::Duration{1ns}, deuca::Duration{2ns}, deuca::Duration{3ns}};

    REQUIRE(deuca::percentile(sorted, 0.0) == deuca::Duration{1ns});
    REQUIRE(deuca::percentile(sorted, -1.0) == deuca::Duration{1ns});
    REQUIRE(deuca::percentile(sorted, 1.0) == deuca::Duration{3ns});
    REQUIRE(deuca::percentile(sorted, 42.0) == deuca::Duration{3ns});
}

TEST_CASE("Le percentile d'un echantillon vide vaut zero", "[stats]")
{
    REQUIRE(deuca::percentile({}, 0.5) == deuca::Duration::zero());
}
