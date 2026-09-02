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

### Campagne du 2 septembre 2026 — avant / après réglage

Chaque cadence est mesurée deux fois dans la **même exécution**, les deux passes
enchaînées : sans réglage d'ordonnancement, puis avec. Deux campagnes séparées
seraient attaquables sur la charge de la machine entre les deux.

Cette campagne remplace celle du 1er septembre, menée avec un minimum de vingt
échantillons dont les percentiles ne voulaient rien dire.

| | |
|---|---|
| Processeur | AMD Ryzen 5 PRO 4650U (6 cœurs / 12 threads, mobile 15 W) |
| Mémoire | 31,2 Go |
| Système | Windows 11 Pro 10.0.26200 |
| Build | `x64-release` (RelWithDebInfo), CRT statique |
| Timer | haute résolution, granularité annoncée 500 µs |
| Marge d'attente active | 500 µs (relevée depuis 300 µs par la granularité) |
| Cœurs logiques | 12, homogènes |
| Leviers acceptés | MMCSS, priorité, sélection de cœurs, refus du bridage : tous |
| Alimentation | secteur |

Retard au réveil, en microsecondes :

| Cadence | Réglage | Échant. | min | p50 | p90 | p99 | max | moyenne |
|---:|:--|---:|---:|---:|---:|---:|---:|---:|
| 10 Hz | brut | 200 | 0,0 | 0,0 | 62,7 | 163,8 | 215,5 | 15,0 |
| 10 Hz | réglé | 200 | 0,0 | 0,0 | 54,8 | 253,8 | 318,9 | 17,3 |
| 100 Hz | brut | 200 | 0,0 | 0,0 | 69,5 | 357,3 | 384,9 | 21,3 |
| 100 Hz | réglé | 200 | 0,0 | 0,0 | 64,7 | 385,4 | 598,9 | 20,3 |
| 500 Hz | brut | 500 | 0,0 | 0,0 | 0,0 | 119,8 | 730,4 | 6,5 |
| 500 Hz | réglé | 500 | 0,0 | 0,0 | 0,0 | 101,6 | 340,5 | 4,0 |
| 1000 Hz | brut | 1000 | 0,0 | 0,0 | 0,0 | 275,7 | 1 103,6 | 11,3 |
| 1000 Hz | réglé | 1000 | 0,0 | 0,0 | 0,0 | 150,4 | 936,3 | 7,0 |
| 2000 Hz | brut | 2000 | 0,0 | 0,0 | 0,0 | 0,0 | 36,7 | 0,0 |
| 2000 Hz | réglé | 2000 | 0,0 | 0,0 | 0,0 | 0,0 | 18,0 | 0,0 |

### Lecture

**L'attente active tient sa promesse.** Le retard médian est de 0,0 µs partout :
la boucle de spin sort dans le même top d'horloge que l'échéance, soit moins de
100 ns sur un QPC à 10 MHz. Ce n'est pas là que se joue l'erreur.

**Toute la queue de distribution vient de l'attente bloquante.** À 2000 Hz, la
période vaut exactement la marge d'attente active : `blockingPortion` renvoie
zéro, le waiter ne bloque jamais, et le maximum tombe à quelques dizaines de
microsecondes. Partout où le timer est sollicité, la queue réapparaît.

**Le réglage d'ordonnancement gagne à partir de 500 Hz, et nulle part ailleurs.**

| Cadence | p99 | max | moyenne |
|---:|---:|---:|---:|
| 500 Hz | −15 % | −53 % | −38 % |
| 1000 Hz | −45 % | −15 % | −38 % |
| 2000 Hz | — | −51 % | — |

En dessous, aucun gain n'est démontrable : à 10 et 100 Hz les chiffres réglés
sont même légèrement moins bons que les bruts. Voir les réserves.

**Ce résultat est cohérent avec ce qui compte réellement.** Rapporté à la
période, le retard maximal réglé vaut 0,3 % à 10 Hz, 6 % à 100 Hz, 17 % à
500 Hz et 94 % à 1000 Hz. Aux cadences lentes, la queue est sans conséquence :
un autoclicker à 10 Hz dispose de 100 ms de marge et trois cents microsecondes
n'y changent rien. Le réglage améliore donc exactement là où l'erreur devient
significative.

**Une limite reste entière.** À 1000 Hz, le retard maximal réglé approche encore
une période entière. L'ordonnancement seul ne suffit pas à cette cadence, et la
piste à explorer n'est pas de pousser la priorité plus loin mais d'élargir la
marge d'attente active : à 2000 Hz, où le waiter ne bloque jamais, le maximum
est cinquante fois plus faible. Un balayage de la marge de spin est le
prochain protocole à mener.

### Réserves

- **Les lignes 10 et 100 Hz reposent sur 200 échantillons.** Le p99 y est porté
  par deux mesures : l'écart observé entre brut et réglé est du même ordre que
  le bruit que cette taille d'échantillon autorise. Il ne faut pas y lire une
  dégradation, seulement une absence de gain démontrable.
- Les lignes 500, 1000 et 2000 Hz reposent sur 500 à 2000 échantillons ; leurs
  écarts sont exploitables.
- La campagne a été menée avec **certains services antivirus temporairement
  désactivés** — voir la note ci-dessous. La charge de fond du poste n'était pas
  contrôlée.
- La sélection de cœurs performants est sans effet sur ce processeur : douze
  cœurs logiques d'une seule classe d'efficacité. Son intérêt ne se mesurera que
  sur une architecture hybride.
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
