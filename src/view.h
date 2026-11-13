#pragma once
#include "gamemodel.h"

#include <QColor>
#include <QFont>

#include <optional>
#include <string>

class QPainter;
class QSizeF;
class QTransform;

// This class acts as the MVC "view" component - its function is to
// update the display according to the state held within the model.
//
// It draws on a canvas supplied by the GameShell, whose size may differ from
// the window's physical size. The view treats it as a window onto the Level
// arena with a constant area (equivalent to 480x300 arena units), whatever
// the window's size or shape.
//
// The "view" also includes other output, e.g. sounds.

namespace marengo {
namespace amaze {

class Audio;

class View {
public:
    // A QApplication must exist before this is constructed
    View(GameModel& model, Audio& audio, const std::string& dataDir);
    // this is called once per displayed frame
    void draw(QPainter& painter, const QSizeF& canvasSize) const;
    void playSounds(); // this is called once per simulation tick
    void stopSounds();

private:
    void
    drawShape(QPainter& painter, const QTransform& arenaTransform, const GameShape& shape) const;
    // x and y are the text's top left; no value means centred in that dimension
    void drawText(
        QPainter& painter,
        const QSizeF& canvasSize,
        const std::string& text,
        const QColor& colour,
        int pixelSize,
        std::optional<double> x,
        std::optional<double> y) const;

    GameModel& m_model;
    Audio& m_audio;
    QFont m_font;
};

} // namespace amaze
} // namespace marengo
