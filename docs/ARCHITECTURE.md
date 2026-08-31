# Architecture

## Couches

Le dépôt est découpé pour que la partie qui mérite d'être testée n'ait pas
besoin d'une session graphique pour tourner, et pour que la partie qui parle au
système d'exploitation reste assez petite pour se lire d'une traite.

| Couche | Dossier | Peut inclure | Rôle |
|---|---|---|---|
| `core` | `src/core` | bibliothèque standard uniquement | Cadence, plans, statistiques. Ni `<Windows.h>`, ni ImGui, ni variable globale. |
| `platform` | `src/platform` | `core`, `<Windows.h>` | La seule couche qui parle au système. Un en-tête par capacité. |
| `ui` | `src/ui` | `core`, `platform`, ImGui | Fenêtre, chaîne d'échange, panneaux. |
| `app` | `src/app` | tout ce qui précède | Point d'entrée et boucle de rendu. Rien d'autre. |

Le sens des dépendances est unique et imposé par les cibles CMake : `core`
ignore tout de `platform`, et `platform` ignore tout de `ui`. Si un changement
exige une flèche dans l'autre sens, c'est la conception qui est fautive, pas la
règle.

ImGui est lié en `PRIVATE` à `deuca_ui` pour la même raison : le point d'entrée
n'hérite pas de son chemin d'inclusion, et la règle des couches est donc
vérifiée par le compilateur plutôt que par la bonne volonté de chacun.

`<Windows.h>` est inclus à un seul endroit, `platform/WindowsLean.hpp`. C'est
toute la différence entre un projet où `std::min` fonctionne et un projet où il
cesse mystérieusement de compiler.

## Compilation

```
cmake --preset x64-debug
cmake --build --preset x64-debug
ctest --preset x64-debug
```

Les dépendances viennent de vcpkg en mode manifeste (`vcpkg.json`) ; le fichier
de chaîne d'outils est récupéré depuis `VCPKG_ROOT`, que Visual Studio
renseigne pour vous. Le triplet `x64-windows-static` et un runtime C statique
donnent un unique `.exe` autonome, sans redistribuable à installer.

Trois presets :

- `x64-debug` — assertions actives, tests compilés.
- `x64-release` — `RelWithDebInfo`, tests compilés. À utiliser pour toute
  mesure : les chiffres d'une build Debug ne veulent rien dire.
- `x64-ship` — `Release` complet avec LTCG, tests désactivés. C'est ce qui est
  publié.

## Conventions

- C++20. `std::jthread` et `std::stop_token` sont structurants, pas décoratifs.
- Avertissements à `/W4` avec `/permissive-`, appliqués via la cible interface
  `deuca::compile_flags` pour que le code tiers ramené par vcpkg ne soit jamais
  soumis à notre niveau d'exigence.
- Le formatage est celui de `.clang-format` (base Microsoft), vérifié en
  intégration continue et bloquant. La version est **épinglée à
  clang-format 23.1.0** : le résultat du formatage varie d'une version à
  l'autre, et sans épinglage la CI finit par refuser du code correct. Celle
  livrée avec Visual Studio n'est pas forcément la bonne — installez la même
  que la CI :

  ```
  pip install clang-format==23.1.0
  clang-format -i --style=file src/**/*.cpp src/**/*.hpp tests/*.cpp
  ```
- Le code, les commentaires et les messages de commit sont en français. Les
  identifiants restent en anglais, comme les API qu'ils enveloppent.
- Un commentaire explique le *pourquoi*, jamais le *quoi*. Un commentaire qui
  paraphrase la ligne en dessous est du bruit ; un commentaire qui consigne le
  comportement du système auquel un contournement répond est ce qu'un fichier
  contient de plus précieux.
- Préfixes de branche : `feat/`, `fix/`, `perf/`, `chore/`, `docs/`,
  `refactor/`, `test/`. Les sujets de commit suivent Conventional Commits.

## État

Le squelette compile une fenêtre, et rien de plus. Le moteur arrive dans les
branches qui suivent.
