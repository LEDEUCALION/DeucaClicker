#pragma once

#include "core/ScreenCoordinates.hpp"

#include <memory>

namespace deuca::platform
{

/// Capture un point de l'écran désigné au clic.
///
/// Le déroulé imite ce que fait tout outil du genre, parce que c'est le seul
/// qui fonctionne : l'application s'efface, l'utilisateur va cliquer là où il
/// veut, le clic est intercepté et **avalé**. Avaler le clic est le point
/// délicat — sans cela, le geste de désignation activerait aussi ce qui se
/// trouve sous le curseur, et désigner un bouton reviendrait à l'actionner.
///
/// L'interception passe par un crochet bas niveau, posé sur le fil appelant.
/// Ce fil doit donc traiter des messages : le système appelle le crochet
/// depuis sa boucle. C'est le cas du fil d'interface, et de lui seul.
class PointPicker
{
public:
    PointPicker() noexcept;
    ~PointPicker();

    PointPicker(const PointPicker&) = delete;
    PointPicker& operator=(const PointPicker&) = delete;
    PointPicker(PointPicker&&) = delete;
    PointPicker& operator=(PointPicker&&) = delete;

    /// Démarre une capture et efface la fenêtre donnée.
    ///
    /// @param window fenêtre à réduire pendant la désignation ; peut être nulle.
    /// @return false si le crochet n'a pas pu être posé, auquel cas rien n'a
    ///         été modifié et la fenêtre reste visible.
    bool begin(void* window);

    /// Interrompt la capture et rétablit la fenêtre.
    void cancel();

    [[nodiscard]] bool isActive() const noexcept;

    /// True si un point a été désigné et n'a pas encore été récupéré.
    [[nodiscard]] bool hasResult() const noexcept;

    /// Récupère le point désigné et remet le collecteur à zéro.
    [[nodiscard]] ScreenPoint takeResult() noexcept;

    /// Position courante du curseur pendant une capture, pour l'affichage.
    [[nodiscard]] ScreenPoint liveCursor() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace deuca::platform
