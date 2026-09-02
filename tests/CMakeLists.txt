# La suite tourne sans session graphique. C'est vrai de core/, qui est du C++
# pur, mais aussi des morceaux de platform/ qui ne créent pas de fenêtre : un
# timer attendable se teste très bien sur un runner sans bureau.
find_package(Catch2 3 CONFIG REQUIRED)

add_executable(deuca_tests
    VersionTests.cpp
    WaitingTests.cpp
    StatisticsTests.cpp
    HighResolutionTimerTests.cpp
    ThreadTuningTests.cpp)

target_link_libraries(deuca_tests PRIVATE
    deuca::core
    deuca::platform
    deuca::compile_flags
    Catch2::Catch2WithMain)

set_target_properties(deuca_tests PROPERTIES FOLDER "tests")

include(Catch)
catch_discover_tests(deuca_tests)
