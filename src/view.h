#pragma once
#include "gamemodel.h"

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRectF>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

class QPainter;
class QSizeF;
class QTextDocument;
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
    ~View();
    // this is called once per displayed frame
    void draw(QPainter& painter, const QSizeF& canvasSize) const;
    void playSounds(); // this is called once per simulation tick
    void stopSounds();

private:
    // Where on the canvas the ship appears
    QPointF viewCentre(const QSizeF& canvasSize) const;
    // A box in the status panel's style
    void drawPanelBox(QPainter& painter, const QRectF& rect, const QColor& background) const;
    void drawStatusPanel(QPainter& painter) const;
    // Before the level starts: "Get Ready", or the level's introduction
    void drawGetReady(QPainter& painter, const QPointF& centre, const QSizeF& canvasSize) const;
    // The level's introduction (with its title), in a panel. Returns the
    // panel's area.
    QRectF
    drawIntroduction(QPainter& painter, const QPointF& centre, const QSizeF& canvasSize) const;
    void
    drawShape(QPainter& painter, const QTransform& arenaTransform, const GameShape& shape) const;
    // Debris falling into objects with gravity
    void drawGravityFields(QPainter& painter, const QTransform& arenaTransform) const;
    // x and y are the text's top left; no value means centred on `centre` in
    // that dimension. Returns the area the text was drawn in.
    QRectF drawText(
        QPainter& painter,
        const QPointF& centre,
        const std::string& text,
        const QColor& colour,
        int pixelSize,
        std::optional<double> x,
        std::optional<double> y) const;

    GameModel& m_model;
    Audio& m_audio;
    QFont m_font;

    // Status panel layout, in canvas pixels
    QRectF m_panelRect;
    double m_panelRowHeight { 0.0 };
    double m_panelTitleHeight { 0.0 };
    double m_panelValueWidth { 0.0 };

    // The thrust bar's fill, which follows the thrust smoothly so that small,
    // rapid changes (e.g. from a gamepad stick) don't make it jitter
    mutable double m_thrustBarFill { 0.0 };
    mutable std::optional<std::chrono::steady_clock::time_point> m_lastPanelDrawTime;

    // The level's introduction, laid out, and the text it was made from, so
    // it's only laid out again when that changes
    mutable std::unique_ptr<QTextDocument> m_introduction;
    mutable std::string m_introductionSource;
};

} // namespace amaze
} // namespace marengo
