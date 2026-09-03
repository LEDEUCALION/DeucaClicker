# DeucaClicker

Un auto-clicker Windows rapide, mesuré, et qui refuse de démarrer sans issue de
secours.

> *English — a fast Windows auto-clicker written in native C++20. Documentation,
> interface and source comments are in French. Build instructions are under
> [Compiler](#compiler).*

---

## Ce que c'est

Un auto-clicker natif pour Windows 10 et 11, écrit en C++20, sans dépendance à
installer : un seul exécutable d'environ deux mégaoctets, aucun redistribuable.

Ce qui le distingue des dizaines d'outils équivalents n'est pas la vitesse
maximale — tout le monde annonce des chiffres invérifiables. C'est **ce qu'il
fait quand les choses se passent mal** : quand l'application visée n'arrive plus
à suivre, quand le raccourci d'arrêt est déjà pris par un autre logiciel, quand
une session a été oubliée en marche.

- Bouton gauche, droit ou milieu. Clic simple ou double.
- Intervalle en heures, minutes, secondes, millisecondes — ou cadence en clics
  par seconde. Les deux affichages restent d'accord.
- Répétition illimitée ou nombre de fois exact.
- Plusieurs points de clic, visités en boucle, capturés à la position du
  curseur.
- Groupement des clics pour les cadences élevées.
- Raccourci global de démarrage et d'arrêt, reconfigurable.

## Sûreté

Trois règles sont inscrites dans le code, pas dans une page d'aide.

**Sans raccourci d'arrêt, pas de démarrage.** Si une autre application détient
déjà la combinaison, le bouton reste inactif et l'interface dit pourquoi. Un
auto-clicker lancé sans moyen de l'arrêter au clavier ne se rattrape qu'à la
souris — avec une souris qui clique toute seule plusieurs centaines de fois par
seconde.

**L'arrêt d'urgence ne dépend pas de l'interface.** Il écoute sur son propre fil
d'exécution, derrière une fenêtre sans affichage. Il répond donc même si
l'interface est occupée ou figée — c'est-à-dire exactement dans le cas où on en
a besoin.

**Une session oubliée s'arrête d'elle-même.** Une durée maximale par défaut, et
un plafond de cadence qui n'est pas une limite technique mais une limite de
responsabilité.

Le raccourci est reconfigurable sans que la garantie cède : le changement retire
l'ancienne combinaison, tente la nouvelle, et remet l'ancienne si le système la
refuse. Il n'existe aucun instant où l'arrêt d'urgence serait absent.

## S'adapter à la cible

Windows fusionne les messages de déplacement de souris quand une file d'attente
s'engorge. Il ne fusionne **pas** les clics. Une application qui travaille à
chaque clic voit donc sa file gonfler sans limite, passe en « ne répond pas », et
garde son retard plusieurs secondes après l'arrêt.

DeucaClicker mesure la réactivité de la fenêtre visée en lui envoyant un message
vide et en chronométrant sa réponse. Le message ne fait rien : tout le temps
mesuré est du temps passé dans sa file d'attente.

Quand la latence monte, la cadence recule vite. Quand elle redescend, la cadence
remonte par petits pas — la même stratégie que le contrôle de congestion des
réseaux, et pour la même raison : quand on ignore où est la limite, tergiverser
laisse la situation empirer.

L'interface affiche la latence mesurée et le pourcentage réellement appliqué.
Quand ça ralentit, elle dit pourquoi.

## Mesures

Les chiffres sont mesurés, pas estimés. La méthode, les conditions et les
réserves sont dans **[docs/BENCHMARKS.md](docs/BENCHMARKS.md)**.

Extrait — effet des réglages d'ordonnancement sur le retard au réveil, mesuré
sur un Ryzen 5 PRO 4650U :

| Cadence | p99 | maximum | moyenne |
|---:|---:|---:|---:|
| 500 Hz | −15 % | −53 % | −38 % |
| 1000 Hz | −45 % | −15 % | −38 % |
| 2000 Hz | — | −51 % | — |

En dessous de 500 Hz, aucun gain n'est démontrable, et c'est écrit tel quel dans
le document. Un gain qu'on ne mesure pas est une croyance.

## Architecture

Quatre couches, dépendances à sens unique, imposées par le système de
compilation et non par la seule discipline.

| Couche | Peut inclure | Rôle |
|---|---|---|
| `core` | bibliothèque standard uniquement | cadence, plans, statistiques |
| `platform` | `core`, en-têtes système | la seule couche qui parle au système |
| `engine` | `core` | la boucle de cadence |
| `ui` | tout ce qui précède | fenêtre et panneaux |
| `app` | tout | point d'entrée, rien d'autre |

`core` et `engine` sont du C++ portable et sans dépendance système. Ils se
testent donc entièrement — boucle de cadence et fil d'exécution compris — sans
qu'un seul test ne clique où que ce soit.

**99 cas de test**, exécutés à chaque proposition de modification.
Détails dans [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Compiler

Prérequis : Visual Studio 2022 ou plus récent avec la charge de travail C++.
CMake, Ninja et vcpkg sont livrés avec — le script prépare la session.

```powershell
. .\scripts\dev-env.ps1
cmake --preset x64-release
cmake --build --preset x64-release
ctest --preset x64-release
```

Les dépendances sont récupérées par vcpkg en mode manifeste, épinglées sur une
version précise. Le préréglage `x64-ship` produit le binaire publié.

## Faux positifs antivirus

Un exécutable console fraîchement compilé, non signé et lié en statique est le
profil que les analyses heuristiques n'aiment pas. Sur au moins un poste,
Kaspersky place le binaire du banc de mesure en quarantaine avec le verdict
`VHO:Backdoor.Win32.Convagent.gen` — un verdict **heuristique**, sans
correspondance de signature.

Le projet n'intègre aucune mesure destinée à échapper à une détection, et n'en
intégrera pas. Les seules parades employées sont vérifiables : aucun
empaquetage, aucune obfuscation, aucun appel non documenté, et une compilation
reproductible depuis l'intégration continue.

## Usage prévu

Accessibilité, automatisation de tâches répétitives, tests d'interface, jeux
incrémentaux hors ligne.

DeucaClicker viole les conditions d'utilisation de la quasi-totalité des jeux en
ligne, et les systèmes anti-triche détectent l'entrée automatisée. **Aucun
contournement de cette détection n'est implémenté.** Ce n'est pas une faiblesse
du projet, c'est sa position.

## Licence

[MIT](LICENSE) — Copyright © 2026 LEDEUCALION.

Le binaire embarque des bibliothèques tierces dont les licences exigent une
attribution. Voir **[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)**.
