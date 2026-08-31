cmake_minimum_required(VERSION 3.25)

project(DeucaClicker
    VERSION 0.1.0
    DESCRIPTION "Un auto-clicker Win32 ultra-rapide et à faible latence."
    HOMEPAGE_URL "https://github.com/LEDEUCALION/DeucaClicker"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "DeucaClicker ne cible que Windows. Ce projet n'a pas de moitié portable.")
endif()

# ---------------------------------------------------------------------------
# Réglages de langage globaux
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Un seul .exe autonome, aucun redistribuable à traîner. Voir docs/ARCHITECTURE.md.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(DEUCA_BUILD_TESTS "Compiler la suite de tests unitaires" ON)

# ---------------------------------------------------------------------------
# Réglages de compilation partagés
#
# Portés par une cible INTERFACE plutôt que par des options globales : chaque
# consommateur y adhère explicitement, et le code tiers ramené par vcpkg n'est
# jamais compilé avec notre niveau d'avertissement.
# ---------------------------------------------------------------------------
add_library(deuca_compile_flags INTERFACE)
add_library(deuca::compile_flags ALIAS deuca_compile_flags)

target_compile_definitions(deuca_compile_flags INTERFACE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    # Windows 10 est le plancher : plusieurs points d'entrée d'ordonnancement et
    # de DPI dont nous dépendons ne sont déclarés qu'à partir de 0x0A00.
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00)

target_compile_options(deuca_compile_flags INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4                 # tout sauf /Wall, qui n'est essentiellement que du bruit
        /permissive-        # mode conforme ; débusque les habitudes propres à MSVC
        /utf-8
        /Zc:__cplusplus     # sans ça, __cplusplus ment et annonce du C++98
        /Zc:preprocessor
        /MP
        /EHsc
        $<$<CONFIG:Release>:/O2 /GL>
    >)

target_link_options(deuca_compile_flags INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/LTCG>>)

add_subdirectory(src)

if(DEUCA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
