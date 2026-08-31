# Fenêtre, chaîne d'échange et amorçage de Dear ImGui. C'est le seul endroit de
# la couche UI qui touche directement à la plateforme ; les panneaux au-dessus
# ne le font jamais.
#
# ImGui est lié en PRIVATE à dessein : app/ ne doit pas hériter de son chemin
# d'inclusion, sans quoi la règle des couches ne serait plus qu'un vœu pieux.
add_library(deuca_ui STATIC
    ImGuiHost.hpp
    ImGuiHost.cpp
    MainPanel.hpp
    MainPanel.cpp)

add_library(deuca::ui ALIAS deuca_ui)

find_package(imgui CONFIG REQUIRED)

target_include_directories(deuca_ui PUBLIC "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(deuca_ui
    PUBLIC  deuca::core
    PRIVATE deuca::platform
            deuca::compile_flags
            imgui::imgui
            d3d11
            dxgi)

set_target_properties(deuca_ui PROPERTIES FOLDER "libs")
