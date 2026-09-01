# C++ pur : pas de <Windows.h>, pas d'ImGui, pas de variable globale. Tout ce
# qui se trouve ici tourne dans un exécutable console sans session graphique,
# ce qui est précisément la raison d'être de cette séparation.
add_library(deuca_core STATIC
    Waiting.hpp
    IBlockingSleeper.hpp
    PrecisionWaiter.hpp
    PrecisionWaiter.cpp
    Version.hpp
    Version.cpp)

add_library(deuca::core ALIAS deuca_core)

target_include_directories(deuca_core PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(deuca_core PRIVATE deuca::compile_flags)

# La version n'existe qu'à un seul endroit : le project() à la racine du dépôt.
target_compile_definitions(deuca_core PRIVATE
    DEUCA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    DEUCA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    DEUCA_VERSION_PATCH=${PROJECT_VERSION_PATCH})

set_target_properties(deuca_core PROPERTIES FOLDER "libs")
