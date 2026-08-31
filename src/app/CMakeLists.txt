# WIN32 plutôt que le sous-système console : pas de fenêtre noire parasite
# au lancement.
add_executable(DeucaClicker WIN32 main.cpp)

target_link_libraries(DeucaClicker PRIVATE
    deuca::core
    deuca::platform
    deuca::ui
    deuca::compile_flags)

set_target_properties(DeucaClicker PROPERTIES
    OUTPUT_NAME "DeucaClicker"
    FOLDER "app")
