#pragma once

// Mapping from arena coordinates to the canvas the View draws on

#include <QSizeF>
#include <QTransform>

#include <cmath>

namespace marengo {
namespace amaze {

// The canvas shows the arena centred on (centreX, centreY), rotated by
// `rotation` degrees about that point, and scaled so that it always shows the
// same area of the arena (480x300 units' worth) whatever the canvas's size or
// shape, so a differently-shaped window doesn't let the player see more of
// the maze.
inline QTransform
arenaToCanvas(double centreX, double centreY, double rotation, const QSizeF& canvasSize)
{
    double width = canvasSize.width();
    double height = canvasSize.height();
    double scale = std::sqrt((width * height) / (480.0 * 300.0));
    // Note QTransform applies these to points in reverse order
    QTransform transform;
    transform.translate(width / 2.0, height / 2.0);
    transform.scale(scale, scale);
    transform.rotate(rotation);
    transform.translate(-centreX, -centreY);
    return transform;
}

} // namespace amaze
} // namespace marengo
