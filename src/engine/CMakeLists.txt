# La boucle de cadence. C++ pur, comme core/ : elle consomme un puits et un
# dormeur abstraits, et reçoit les réglages d'ordonnancement par une fabrique
# opaque. Aucune ligne de Win32 ici, et donc un moteur vérifiable avec des
# doublures, fil d'exécution compris.
add_library(deuca_engine STATIC
    ClickEngine.hpp
    ClickEngine.cpp)

add_library(deuca::engine ALIAS deuca_engine)

target_include_directories(deuca_engine PUBLIC "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(deuca_engine
    PUBLIC  deuca::core
    PRIVATE deuca::compile_flags)

set_target_properties(deuca_engine PROPERTIES FOLDER "libs")
