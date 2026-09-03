# La seule couche autorisée à inclure <Windows.h>. Tout ce dont le reste du
# code a besoin du système d'exploitation passe par un en-tête déclaré ici.
add_library(deuca_platform STATIC
    WindowsLean.hpp
    HighResolutionTimer.hpp
    HighResolutionTimer.cpp
    CpuTopology.hpp
    CpuTopology.cpp
    ThreadTuning.hpp
    ThreadTuning.cpp
    Hotkey.hpp
    Hotkey.cpp
    HotkeyService.hpp
    HotkeyService.cpp
    Cursor.hpp
    Cursor.cpp
    SendInputSink.hpp
    SendInputSink.cpp
    ProcessSetup.hpp
    ProcessSetup.cpp)

add_library(deuca::platform ALIAS deuca_platform)

target_include_directories(deuca_platform PUBLIC "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(deuca_platform
    PUBLIC  deuca::core
    PRIVATE deuca::compile_flags)

# avrt porte MMCSS, le service d'ordonnancement multimédia.
target_link_libraries(deuca_platform PUBLIC user32 avrt)

set_target_properties(deuca_platform PROPERTIES FOLDER "libs")
