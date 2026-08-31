# core/ est du C++ pur : la suite tourne sans session graphique, donc sans
# bricolage dans l'intégration continue.
find_package(Catch2 3 CONFIG REQUIRED)

add_executable(deuca_tests VersionTests.cpp)

target_link_libraries(deuca_tests PRIVATE
    deuca::core
    deuca::compile_flags
    Catch2::Catch2WithMain)

set_target_properties(deuca_tests PROPERTIES FOLDER "tests")

include(Catch)
catch_discover_tests(deuca_tests)
