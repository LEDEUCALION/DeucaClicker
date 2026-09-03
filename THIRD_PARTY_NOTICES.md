# Mentions relatives aux bibliothèques tierces

DeucaClicker est distribué sous licence [MIT](LICENSE). Le binaire publié est
lié **statiquement**, il embarque donc le code des bibliothèques listées
ci-dessous. Toutes sont sous licence permissive et compatibles avec la licence
du projet ; toutes exigent en revanche que leur attribution accompagne la
distribution.

Ce fichier est cette attribution. Il doit accompagner tout binaire publié.

Les versions indiquées sont celles épinglées dans [`vcpkg.json`](vcpkg.json).
Elles changent à chaque mise à jour du manifeste ; ce fichier est alors à
reprendre.

---

## Dear ImGui — 1.92.8

Interface graphique en mode immédiat.

Copyright © 2014-2026 Omar Cornut. Licence MIT.

<https://github.com/ocornut/imgui>

---

## FreeType — 2.14.3

Rendu des polices de caractères.

Portions of this software are copyright © 2026 The FreeType Project
(<https://www.freetype.org>). All rights reserved.

Distribué sous la **FreeType License (FTL)**, une licence de type BSD. Elle est
la seule du lot à exiger explicitement que le crédit apparaisse dans la
documentation du programme qui l'utilise — d'où la formulation ci-dessus, qui
est celle demandée par le projet FreeType.

<https://freetype.org/license.html>

---

## libpng — 1.6.58

Lecture et écriture d'images PNG, dépendance de FreeType.

Copyright © 1995-2026 The PNG Reference Library Authors. Licence libpng
(dérivée de zlib).

<http://www.libpng.org/pub/png/libpng.html>

---

## zlib — 1.3.2

Compression, dépendance de libpng et de FreeType.

Copyright © 1995-2026 Jean-loup Gailly et Mark Adler. Licence zlib.

<https://zlib.net>

---

## Brotli — 1.2.0

Compression, dépendance de FreeType.

Copyright © 2009, 2010, 2013-2016 by the Brotli Authors. Licence MIT.

<https://github.com/google/brotli>

---

## bzip2 — 1.0.8

Compression, dépendance de FreeType.

Copyright © 1996-2019 Julian R Seward. Licence bzip2 (type BSD).

<https://sourceware.org/bzip2/>

---

## Non embarqué dans le binaire publié

**Catch2** (3.15.3, licence BSL-1.0) sert uniquement à la suite de tests. Il
n'entre pas dans le préréglage `x64-ship` et n'a donc pas à être crédité auprès
des utilisateurs — la mention figure ici par honnêteté d'inventaire.

<https://github.com/catchorg/Catch2>

---

## Texte intégral des licences

Le texte complet de chaque licence est installé par vcpkg à côté de la
bibliothèque correspondante :

```
vcpkg_installed/x64-windows-static/share/<bibliothèque>/copyright
```

Ces fichiers font foi. Le présent document en est le résumé, pas le
remplacement.
