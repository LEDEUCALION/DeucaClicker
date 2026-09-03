#include "engine/ClickEngine.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{

/// Puits simulé : il compte ce qu'on lui soumet et n'injecte rien.
///
/// C'est ce qui permet de vérifier la boucle de cadence sur la machine d'un
/// contributeur sans que son curseur bouge d'un pixel.
class FakeSink final : public deuca::IInputSink
{
public:
    explicit FakeSink(std::size_t maxBatch = 64, std::size_t acceptedPerCall = 0) noexcept
        : m_maxBatch{maxBatch}, m_acceptedOverride{acceptedPerCall}
    {
    }

    std::size_t submit(std::span<const deuca::ClickEvent> events) override
    {
        const std::lock_guard guard{m_mutex};
        ++m_submissions;
        m_lastSize = events.size();

        if (m_acceptedOverride > 0)
        {
            return std::min(m_acceptedOverride, events.size());
        }

        return events.size();
    }

    [[nodiscard]] std::size_t maxBatchSize() const noexcept override { return m_maxBatch; }

    [[nodiscard]] std::size_t submissions() const
    {
        const std::lock_guard guard{m_mutex};
        return m_submissions;
    }

    [[nodiscard]] std::size_t lastSize() const
    {
        const std::lock_guard guard{m_mutex};
        return m_lastSize;
    }

private:
    mutable std::mutex m_mutex;
    std::size_t m_maxBatch;
    std::size_t m_acceptedOverride;
    std::size_t m_submissions{0};
    std::size_t m_lastSize{0};
};

/// Dormeur simulé qui dort réellement : sans cela le waiter terminerait chaque
/// attente en actif et la suite brûlerait un cœur pour rien.
class RealSleeper final : public deuca::IBlockingSleeper
{
public:
    [[nodiscard]] deuca::Duration granularity() const noexcept override { return deuca::Duration{500us}; }

    void sleepFor(deuca::Duration duration) override
    {
        if (duration > deuca::Duration::zero())
        {
            std::this_thread::sleep_for(duration);
        }
    }
};

/// Attend qu'une condition devienne vraie, sans dépasser le délai imparti.
template <typename Predicate>
[[nodiscard]] bool waitFor(Predicate predicate, deuca::Duration timeout)
{
    const deuca::Timestamp limit = deuca::Clock::now() + timeout;
    while (deuca::Clock::now() < limit)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }

    return predicate();
}

} // namespace

TEST_CASE("Le plan est ramene sous le plafond de cadence", "[engine]")
{
    deuca::EngineConfig config;
    config.maxClicksPerSecond = 1000.0;

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 50000.0;

    const deuca::ClickPlan clamped = deuca::clampPlan(plan, config, 64);
    REQUIRE(clamped.clicksPerSecond == 1000.0);
}

TEST_CASE("Une cadence sous le plafond n'est pas touchee", "[engine]")
{
    deuca::EngineConfig config;
    config.maxClicksPerSecond = 1000.0;

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 42.0;

    REQUIRE(deuca::clampPlan(plan, config, 64).clicksPerSecond == 42.0);
}

TEST_CASE("Le lot est ramene a ce que le puits peut avaler", "[engine]")
{
    const deuca::EngineConfig config;

    deuca::ClickPlan plan;
    plan.burstSize = 100;

    // Dix evenements par lot, deux par clic : cinq clics au maximum.
    const deuca::ClickPlan clamped = deuca::clampPlan(plan, config, 10);
    REQUIRE(clamped.burstSize == 5);
    REQUIRE(deuca::eventsPerBurst(clamped) <= 10);
}

TEST_CASE("Un lot de zero clic est ramene a un", "[engine]")
{
    const deuca::EngineConfig config;

    deuca::ClickPlan plan;
    plan.burstSize = 0;

    REQUIRE(deuca::clampPlan(plan, config, 64).burstSize == 1);
}

TEST_CASE("Un plan sans cadence ne demarre pas", "[engine]")
{
    FakeSink sink;
    RealSleeper sleeper;
    deuca::ClickEngine engine{sink, sleeper};

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 0.0;

    engine.start(plan);

    // Refuser de demarrer vaut mieux que tourner a vide en laissant croire le
    // contraire.
    REQUIRE_FALSE(engine.isRunning());
    REQUIRE(sink.submissions() == 0);
}

TEST_CASE("Le moteur soumet des lots puis s'arrete proprement", "[engine]")
{
    FakeSink sink;
    RealSleeper sleeper;
    deuca::ClickEngine engine{sink, sleeper};

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 200.0;
    plan.burstSize = 2;

    engine.start(plan);

    REQUIRE(waitFor([&] { return sink.submissions() >= 3; }, deuca::Duration{2s}));

    engine.stop();
    REQUIRE_FALSE(engine.isRunning());

    const deuca::EngineSnapshot snapshot = engine.snapshot();
    REQUIRE(snapshot.burstsSubmitted >= 3);
    REQUIRE(snapshot.clicksEmitted >= 6);
    REQUIRE(sink.lastSize() == 4);
}

TEST_CASE("L'homme mort arrete une session oubliee", "[engine]")
{
    FakeSink sink;
    RealSleeper sleeper;

    deuca::EngineConfig config;
    config.maxRunDuration = deuca::Duration{80ms};

    deuca::ClickEngine engine{sink, sleeper, config};

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 200.0;

    engine.start(plan);

    REQUIRE(waitFor([&] { return !engine.isRunning(); }, deuca::Duration{3s}));
}

TEST_CASE("Un refus du puits est compte et non ignore", "[engine]")
{
    // Le puits accepte zero evenement : c'est ce que renvoie le systeme quand
    // l'entree est bloquee par un autre processus.
    FakeSink sink{64, 1};
    RealSleeper sleeper;
    deuca::ClickEngine engine{sink, sleeper};

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 200.0;
    plan.burstSize = 2;

    engine.start(plan);
    REQUIRE(waitFor([&] { return engine.snapshot().eventsRejected > 0; }, deuca::Duration{2s}));
    engine.stop();
}

TEST_CASE("La preparation du fil est posee puis relachee", "[engine]")
{
    FakeSink sink;
    RealSleeper sleeper;
    deuca::ClickEngine engine{sink, sleeper};

    std::atomic<int> applied{0};
    std::atomic<int> released{0};

    engine.setThreadPreparation([&] {
        applied.fetch_add(1);
        // Le deleter du shared_ptr joue le role du destructeur RAII : c'est
        // ainsi qu'un reglage pose sur le fil est defait a son arret.
        return std::shared_ptr<void>{nullptr, [&](void*) { released.fetch_add(1); }};
    });

    deuca::ClickPlan plan;
    plan.clicksPerSecond = 200.0;

    engine.start(plan);
    REQUIRE(waitFor([&] { return applied.load() == 1; }, deuca::Duration{2s}));

    engine.stop();
    REQUIRE(released.load() == 1);
}

TEST_CASE("La destruction arrete le moteur en marche", "[engine]")
{
    FakeSink sink;
    RealSleeper sleeper;

    {
        deuca::ClickEngine engine{sink, sleeper};

        deuca::ClickPlan plan;
        plan.clicksPerSecond = 200.0;

        engine.start(plan);
        REQUIRE(waitFor([&] { return engine.isRunning(); }, deuca::Duration{2s}));
    }

    // On sort du bloc sans avoir appele stop : le destructeur doit joindre le
    // fil plutot que de laisser le programme se terminer avec un thread vivant.
    SUCCEED();
}

TEST_CASE("Sans arret d'urgence, le demarrage est refuse", "[engine][safety]")
{
    deuca::ClickPlan plan;
    plan.clicksPerSecond = 10.0;

    // La regle de surete la plus importante du projet. Un autoclicker lance
    // sans moyen de l'arreter au clavier ne se rattrape qu'a la souris, avec
    // une souris qui clique toute seule.
    REQUIRE(deuca::evaluateStart(false, plan, false) == deuca::StartRefusal::NoPanicHotkey);
}

TEST_CASE("Avec arret d'urgence et cadence valide, le demarrage est autorise", "[engine][safety]")
{
    deuca::ClickPlan plan;
    plan.clicksPerSecond = 10.0;

    REQUIRE(deuca::evaluateStart(true, plan, false) == deuca::StartRefusal::None);
}

TEST_CASE("Une cadence nulle est refusee", "[engine][safety]")
{
    deuca::ClickPlan plan;
    plan.clicksPerSecond = 0.0;

    REQUIRE(deuca::evaluateStart(true, plan, false) == deuca::StartRefusal::InvalidRate);
}

TEST_CASE("Une session deja en cours est signalee avant tout le reste", "[engine][safety]")
{
    deuca::ClickPlan plan;
    plan.clicksPerSecond = 0.0;

    // Meme avec un plan invalide et sans arret d'urgence : dire « ca tourne
    // deja » est le motif le plus utile a l'utilisateur.
    REQUIRE(deuca::evaluateStart(false, plan, true) == deuca::StartRefusal::AlreadyRunning);
}
