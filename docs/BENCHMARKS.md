# Mesures

## Ce que mesure le banc

`deuca_bench` mesure le **retard au réveil** du `PrecisionWaiter` : l'écart entre
l'échéance demandée et l'instant où l'attente rend réellement la main.

C'est la seule grandeur qui compte pour un autoclicker. Une cadence annoncée
n'a de valeur que si les échéances sont tenues ; un outil qui vise 1000 clics
par seconde en se réveillant systématiquement 3 ms trop tard n'en produit que
250, et personne ne le sait.

Le retard est toujours positif ou nul, par construction : `waitUntil` termine en
attente active et ne rend la main qu'une fois l'échéance atteinte.

## Méthode

- **Échéances absolues.** Chaque échéance est calculée depuis un instant de
  départ fixe, jamais cumulée d'un tour sur l'autre. Cumuler ferait reporter le
  retard de chaque tour sur le suivant : on mesurerait une dérive au lieu d'une
  gigue.
- **Rodage.** Cinq tours non mesurés en tête de série, le temps que les caches
  et le timer se mettent en régime.
- **Cadences balayées.** 10, 100, 500, 1000 et 2000 Hz.
- **Échantillons.** Un par tour, avec un minimum de 20 pour que les cadences
  lentes restent interprétables.
- **Percentiles par rang le plus proche**, sans interpolation :
  `index = ceil(fraction × n) − 1` sur l'échantillon trié. Interpoler entre deux
  voisins inventerait une latence que la machine n'a jamais produite.

## Comment exécuter

```
cmake --preset x64-release
cmake --build --preset x64-release
build\x64-release\bin\deuca_bench.exe
```

Le preset `x64-release` et pas `x64-debug` : les chiffres d'une build Debug ne
veulent rien dire. Le banc imprime sa configuration de build en en-tête et
refuse de laisser croire l'inverse.

Notez avec les résultats la machine, la version de Windows, et ce qui tournait à
côté. Une mesure de latence sans son contexte n'est pas une mesure.

## Résultats

### Campagne de référence — 1er septembre 2026

Mesure prise **avant tout réglage d'ordonnancement**. Elle sert de point de
comparaison aux branches qui toucheront à la priorité de thread, à MMCSS et au
bridage énergétique.

| | |
|---|---|
| Processeur | AMD Ryzen 5 PRO 4650U (6 cœurs / 12 threads, mobile 15 W) |
| Mémoire | 31,2 Go |
| Système | Windows 11 Pro 10.0.26200 |
| Build | `x64-release` (RelWithDebInfo), CRT statique |
| Timer | haute résolution, granularité annoncée 500 µs |
| Marge d'attente active | 500 µs (relevée depuis 300 µs par la granularité) |
| Alimentation | secteur |

Retard au réveil, en microsecondes :

| Cadence | Période | Échant. | min | p50 | p90 | p99 | max | moyenne |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 10 Hz | 100 000 µs | 20 | 0,0 | 0,0 | 283,9 | 473,8 | 473,8 | 74,5 |
| 100 Hz | 10 000 µs | 100 | 0,0 | 0,0 | 173,9 | 442,3 | 505,0 | 53,2 |
| 500 Hz | 2 000 µs | 500 | 0,0 | 0,0 | 0,0 | 110,3 | 1 851,5 | 8,0 |
| 1000 Hz | 1 000 µs | 1000 | 0,0 | 0,0 | 0,0 | 180,1 | 1 040,7 | 7,8 |
| 2000 Hz | 500 µs | 2000 | 0,0 | 0,0 | 0,0 | 0,0 | 50,9 | 0,1 |

### Lecture

**L'attente active tient sa promesse.** Le retard médian est de 0,0 µs à toutes
les cadences : la boucle de spin sort dans le même top d'horloge que l'échéance,
soit moins de 100 ns sur un QPC à 10 MHz. Ce n'est pas là que se joue l'erreur.

**Toute la queue de distribution vient de l'attente bloquante.** À 2000 Hz, la
période vaut exactement la marge d'attente active : `blockingPortion` renvoie
zéro, le waiter ne bloque jamais, et le maximum s'effondre à 51 µs pour une
moyenne de 0,1 µs. Partout où le timer est sollicité, la queue réapparaît —
jusqu'à 1,85 ms à 500 Hz, soit près d'une période entière manquée.

Autrement dit, l'erreur n'est pas dans la façon dont on attend les dernières
microsecondes, mais dans le délai de reprise du thread après le déclenchement du
timer. C'est précisément ce sur quoi agissent la priorité de thread, MMCSS et la
désactivation du bridage énergétique — et sur une puce mobile 15 W, où le
gouverneur de fréquence est le plus intrusif, la marge de progression devrait
être visible.

### Réserves

- **La ligne 10 Hz ne vaut rien en percentile.** Vingt échantillons : le rang le
  plus proche fait pointer p99 sur le dernier élément, donc sur le maximum.
  C'est un défaut du banc, pas de la mesure ; le minimum d'échantillons a été
  porté à 200 pour les campagnes suivantes. La ligne 100 Hz est à lire avec la
  même prudence.
- Les lignes 500, 1000 et 2000 Hz reposent sur 500 à 2000 échantillons et sont
  exploitables.
- La campagne a été menée avec **certains services antivirus temporairement
  désactivés** — voir la note ci-dessous. La charge de fond du poste n'était pas
  contrôlée.
- Machine portable sur secteur. Les mêmes mesures sur batterie donneront presque
  certainement une queue plus lourde.

## Note d'exécution : faux positif antivirus

Sur au moins un poste de développement, le binaire du banc est bloqué quelques
secondes après son édition de liens, alors que les autres exécutables du même
dossier ne sont pas touchés. Le lancement échoue sur « Accès refusé » et le
fichier finit par disparaître.

Verdict relevé, Kaspersky :

```
Nom        : VHO:Backdoor.Win32.Convagent.gen
Exactitude : Analyse heuristique
Niveau     : Eleve
```

Le préfixe `VHO:` désigne le verdict de l'analyseur heuristique et le suffixe
`.gen` une règle générique : **aucune signature ne correspond**. C'est un
jugement sur la forme du fichier, pas sur son contenu — et la forme en question
est celle de n'importe quel exécutable console fraîchement compilé, lié en
statique et non signé.

Si le banc disparaît après compilation, cherchez ce verdict dans le journal de
votre antivirus avant de conclure à un bug de build, puis ajoutez une exclusion
sur le dossier `build/`. Il ne contient que des artefacts régénérables et il est
déjà ignoré par Git.

Le même profil s'appliquera au binaire publié. Les parades prévues sont
publiques et vérifiables — aucun packer, aucune obfuscation, aucun appel non
documenté, builds reproductibles depuis l'intégration continue, hachages
publiés, et signature du binaire dès qu'un certificat sera disponible. Aucune
mesure destinée à échapper à une détection n'est intégrée au projet, et il n'y
en aura pas.
