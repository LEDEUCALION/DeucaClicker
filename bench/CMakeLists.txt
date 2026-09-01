# Banc de mesure : exécutable console, pas WIN32. Il doit pouvoir écrire dans
# un terminal et être redirigé vers un fichier.
add_executable(deuca_bench main.cpp)

target_link_libraries(deuca_bench PRIVATE
    deuca::core
    deuca::platform
    deuca::compile_flags)

# La configuration de build est imprimée dans l'en-tête du rapport. Les
# chiffres d'une build Debug ne veulent rien dire, et le lecteur doit le voir
# sans avoir à le demander.
target_compile_definitions(deuca_bench PRIVATE DEUCA_BUILD_TYPE="$<CONFIG>")

set_target_properties(deuca_bench PROPERTIES FOLDER "bench")
