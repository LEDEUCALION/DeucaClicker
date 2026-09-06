#pragma once

#include "core/ScreenCoordinates.hpp"

#include <memory>

namespace deuca::platform
{

/// Repère flottant affiché pendant la désignation d'un point.
///
/// Sans lui, l'application se réduit et l'utilisateur se retrouve devant son
/// bureau sans rien pour lui dire ce qu'on attend de lui. Le repère suit le
/// curseur, affiche les coordonnées visées et rappelle les deux gestes
/// possibles.
///
/// La fenêtre est transparente aux clics et ne prend jamais le focus : elle
/// doit pouvoir survoler la cible sans jamais s'interposer entre le curseur et
/// ce que l'utilisateur veut désigner.
class CaptureOverlay
{
public:
    CaptureOverlay() noexcept;
    ~CaptureOverlay();

    CaptureOverlay(const CaptureOverlay&) = delete;
    CaptureOverlay& operator=(const CaptureOverlay&) = delete;
    CaptureOverlay(CaptureOverlay&&) = delete;
    CaptureOverlay& operator=(CaptureOverlay&&) = delete;

    /// Affiche le repère à côté du point donné.
    ///
    /// @return false si la fenêtre n'a pas pu être créée. La désignation reste
    ///         alors possible, simplement sans guide visuel.
    bool show(ScreenPoint cursor);

    /// Déplace le repère et met ses coordonnées à jour.
    void moveTo(ScreenPoint cursor);

    void hide();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace deuca::platform
