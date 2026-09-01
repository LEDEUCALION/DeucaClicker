#pragma once

#include <chrono>

namespace deuca
{

/// L'horloge de référence du projet.
///
/// C'est volontairement celle de la bibliothèque standard, et non une
/// enveloppe maison autour de QueryPerformanceCounter : sous MSVC,
/// steady_clock est déjà implémentée sur QPC. Réécrire cette couche
/// reviendrait à atterrir sur exactement le même compteur en abandonnant au
/// passage une API testable, et en interdisant à core/ de rester du C++ pur.
///
/// Le prix est une division pour convertir les tops en nanosecondes. C'est un
/// chiffre à mesurer au banc, pas à supposer.
using Clock = std::chrono::steady_clock;

/// Un instant sur cette horloge.
using Timestamp = Clock::time_point;

/// Une durée sur cette horloge. Nanosecondes sous MSVC.
using Duration = Clock::duration;

} // namespace deuca
