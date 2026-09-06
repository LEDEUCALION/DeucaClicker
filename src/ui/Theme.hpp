#pragma once

#include <imgui.h>

#include <span>

/// Système visuel « Noir Fluor ».
///
/// Toutes les valeurs de ce fichier viennent d'une spécification chiffrée. Une
/// valeur absente d'ici est une valeur inventée : elle n'a pas sa place dans le
/// code de dessin.
///
/// Le style repose sur une contrainte du moteur qu'il faut avoir en tête pour
/// lire la suite : **ImGui ne sait pas flouter**. Une ombre douce n'existe pas,
/// elle s'imite en empilant des rectangles arrondis concentriques d'alpha
/// croissant vers le centre. Une ombre intérieure s'imite par deux liserés de
/// un pixel, clair en haut et sombre en bas. Une lueur est une ombre teintée
/// sans décalage.
namespace deuca::ui::theme
{

// ---------------------------------------------------------------------------
// Couleurs
// ---------------------------------------------------------------------------

namespace colour
{

inline constexpr ImU32 abysse = IM_COL32(0x05, 0x07, 0x06, 255);
inline constexpr ImU32 base = IM_COL32(0x0A, 0x0E, 0x0C, 255);
inline constexpr ImU32 creux = IM_COL32(0x07, 0x0A, 0x09, 255);
inline constexpr ImU32 carte = IM_COL32(0x10, 0x16, 0x13, 255);
inline constexpr ImU32 carteHaute = IM_COL32(0x14, 0x1C, 0x18, 255);
inline constexpr ImU32 controle = IM_COL32(0x16, 0x20, 0x1B, 255);

/// Bordure d'un élément **décoratif**. Contraste volontairement faible.
inline constexpr ImU32 filet = IM_COL32(0x1E, 0x2A, 0x24, 255);
inline constexpr ImU32 filetFort = IM_COL32(0x2C, 0x3D, 0x34, 255);

/// Bordure d'un élément **cliquable**, à plus de 3:1 sur le fond.
///
/// La distinction entre ces deux bordures est la parade centrale du système
/// contre le défaut signature du néomorphisme : quand tout est en relief, un
/// bouton ressemble à une carte et l'utilisateur clique au hasard.
inline constexpr ImU32 bordControle = IM_COL32(0x5F, 0x72, 0x68, 255);

inline constexpr ImU32 encre = IM_COL32(0xE9, 0xEF, 0xEB, 255);
inline constexpr ImU32 sourdine = IM_COL32(0x9A, 0xA8, 0xA0, 255);
inline constexpr ImU32 tertiaire = IM_COL32(0x7C, 0x8C, 0x84, 255);
inline constexpr ImU32 eteint = IM_COL32(0x4E, 0x5B, 0x55, 255);

/// Libellé d'un bouton désactivé.
///
/// Relevé à 5,18:1 après calcul de contraste : c'est le texte qui explique
/// pourquoi un démarrage est refusé, il ne peut pas être illisible.
inline constexpr ImU32 eteintLisible = IM_COL32(0x8A, 0x9C, 0x92, 255);

inline constexpr ImU32 accent = IM_COL32(0x22, 0xC5, 0x5E, 255);
inline constexpr ImU32 accentClair = IM_COL32(0x4A, 0xDE, 0x80, 255);
inline constexpr ImU32 accentFonce = IM_COL32(0x16, 0xA3, 0x4A, 255);
inline constexpr ImU32 lueur = IM_COL32(0x39, 0xFF, 0x14, 255);

inline constexpr ImU32 alerte = IM_COL32(0xFB, 0xBF, 0x24, 255);
inline constexpr ImU32 alerteFond = IM_COL32(0x24, 0x1A, 0x05, 255);
inline constexpr ImU32 alerteBord = IM_COL32(0xC0, 0x8A, 0x1E, 255);

inline constexpr ImU32 danger = IM_COL32(0xFF, 0x6B, 0x6B, 255);
inline constexpr ImU32 dangerPlein = IM_COL32(0xE5, 0x48, 0x4D, 255);

/// Texte posé sur un aplat de couleur pleine.
///
/// Toujours sombre, jamais blanc : le blanc sur le rouge d'arrêt tombe à
/// 3,56:1, sous le seuil. Ce texte-ci donne 5,11:1.
inline constexpr ImU32 encreSurAplat = IM_COL32(0x10, 0x06, 0x06, 255);
inline constexpr ImU32 encreSurAccent = IM_COL32(0x04, 0x10, 0x07, 255);

/// Liserés du biseau, qui imitent une ombre intérieure.
inline constexpr ImU32 biseauClair = IM_COL32(0x20, 0x2D, 0x26, 255);
inline constexpr ImU32 biseauSombre = IM_COL32(0x00, 0x00, 0x00, 120);
inline constexpr ImU32 creuxHaut = IM_COL32(0x00, 0x00, 0x00, 160);

} // namespace colour

// ---------------------------------------------------------------------------
// Échelle
// ---------------------------------------------------------------------------

namespace metric
{

// Base 4, avec un pas de 2 conservé pour les liserés et les anneaux.
inline constexpr float esp2 = 2.0f;
inline constexpr float esp4 = 4.0f;
inline constexpr float esp8 = 8.0f;
inline constexpr float esp10 = 10.0f;
inline constexpr float esp12 = 12.0f;
inline constexpr float esp14 = 14.0f;
inline constexpr float esp16 = 16.0f;
inline constexpr float esp20 = 20.0f;
inline constexpr float esp24 = 24.0f;
inline constexpr float esp28 = 28.0f;
inline constexpr float esp32 = 32.0f;
inline constexpr float esp40 = 40.0f;
inline constexpr float esp56 = 56.0f;

inline constexpr float ray5 = 5.0f;
inline constexpr float ray6 = 6.0f;
inline constexpr float ray8 = 8.0f;
inline constexpr float ray10 = 10.0f;
inline constexpr float ray12 = 12.0f;
inline constexpr float ray14 = 14.0f;

/// Gabarit de la fenêtre, vérifié à l'arithmétique :
/// 20 + 516 + 16 + 288 + 20 = 860 et 20 + 96 + 16 + 440 + 14 + 22 + 12 = 620.
inline constexpr float fenetreLargeur = 860.0f;
inline constexpr float fenetreHauteur = 620.0f;
inline constexpr float colonneGauche = 516.0f;
inline constexpr float colonneDroite = 288.0f;
inline constexpr float bandeauHauteur = 96.0f;

} // namespace metric

// ---------------------------------------------------------------------------
// Ombres
// ---------------------------------------------------------------------------

/// Une couche d'une pile d'ombre.
///
/// La couche s'étend de `expand` pixels de chaque côté et se décale de
/// (dx, dy). Empiler plusieurs couches d'alpha croissant vers le centre donne
/// un dégradé par paliers — la seule imitation de flou possible ici.
struct ShadowLayer
{
    float expand{};
    float dx{};
    float dy{};
    float radius{};
    ImU32 colour{};
};

/// S1 — carte en relief. Cartes de section, popup de liste déroulante.
inline constexpr ShadowLayer kCardShadow[]{
    {8.0f, 0.0f, 6.0f, 22.0f, IM_COL32(0, 0, 0, 12)}, {6.0f, 0.0f, 5.0f, 20.0f, IM_COL32(0, 0, 0, 18)},
    {4.0f, 0.0f, 4.0f, 18.0f, IM_COL32(0, 0, 0, 26)}, {2.0f, 0.0f, 3.0f, 16.0f, IM_COL32(0, 0, 0, 36)},
    {1.0f, 0.0f, 2.0f, 15.0f, IM_COL32(0, 0, 0, 48)},
};

/// S2 — bouton principal au repos.
inline constexpr ShadowLayer kPrimaryShadow[]{
    {10.0f, 0.0f, 8.0f, 22.0f, IM_COL32(0, 0, 0, 14)}, {8.0f, 0.0f, 7.0f, 20.0f, IM_COL32(0, 0, 0, 20)},
    {6.0f, 0.0f, 6.0f, 18.0f, IM_COL32(0, 0, 0, 28)},  {4.0f, 0.0f, 5.0f, 16.0f, IM_COL32(0, 0, 0, 38)},
    {2.0f, 0.0f, 4.0f, 14.0f, IM_COL32(0, 0, 0, 52)},  {1.0f, 0.0f, 3.0f, 13.0f, IM_COL32(0, 0, 0, 68)},
};

/// S3 — lueur d'accent. Sans décalage : une lueur ne tombe pas.
inline constexpr ShadowLayer kGlowShadow[]{
    {12.0f, 0.0f, 0.0f, 24.0f, IM_COL32(0x39, 0xFF, 0x14, 10)},
    {8.0f, 0.0f, 0.0f, 20.0f, IM_COL32(0x39, 0xFF, 0x14, 16)},
    {5.0f, 0.0f, 0.0f, 17.0f, IM_COL32(0x39, 0xFF, 0x14, 24)},
    {2.0f, 0.0f, 0.0f, 14.0f, IM_COL32(0x22, 0xC5, 0x5E, 36)},
};

/// S0 — bouton secondaire.
inline constexpr ShadowLayer kSecondaryShadow[]{
    {2.0f, 0.0f, 3.0f, 10.0f, IM_COL32(0, 0, 0, 22)},
    {1.0f, 0.0f, 2.0f, 9.0f, IM_COL32(0, 0, 0, 40)},
};

/// Dessine une pile d'ombre derrière le rectangle donné.
///
/// À appeler **avant** le remplissage : la pile se peint dessous.
/// @param intensity multiplicateur d'alpha, pour les états pressé ou survolé.
void drawShadow(ImDrawList& list, ImVec2 min, ImVec2 max, std::span<const ShadowLayer> layers,
                float intensity = 1.0f);

/// Remplit un rectangle avec le biseau qui imite une ombre intérieure.
///
/// Trois appels : un liseré, son opposé, puis le remplissage. Un trait clair de
/// un pixel en haut et sombre en bas donne le relief ; l'inverse donne le
/// creux. C'est la seule imitation qui reste nette à toute échelle DPI, un
/// dégradé de deux pixels devenant invisible à 100 % et baveux à 200 %.
void fillBevelled(ImDrawList& list, ImVec2 min, ImVec2 max, float radius, ImU32 fill, bool raised);

/// Applique les couleurs et les métriques du système au style ImGui.
///
/// Couvre tout ce que les composants standard savent lire. Ce qui n'est pas
/// exprimable par le style — ombres, biseaux, lueurs — est dessiné à la main
/// par les fonctions ci-dessus.
void applyStyle(float scale);

} // namespace deuca::ui::theme
