#include "view.h"
#include "audio.h"
#include "exceptions.h"
#include "shape.h"
#include "utils.h"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace marengo {
namespace amaze {

namespace {

// Line thicknesses in shapes are multiplied by this. (Their values were tuned
// on a 1920x1200 screen, whose height/1080 this is; the canvas always has
// the same area as that screen.)
constexpr double LINE_THICKNESS_SCALE = 1200.0 / 1080.0;

} // namespace

View::View(GameModel& model, Audio& audio, const std::string& dataDir)
    : m_model(model)
    , m_audio(audio)
{
    int fontId = QFontDatabase::addApplicationFont(
        QString::fromStdString(dataDir + "/Oxanium-SemiBold.ttf"));
    if (fontId < 0) {
        THROWUP(AmazeRuntimeException, "Font file load error");
    }
    m_font = QFont(QFontDatabase::applicationFontFamilies(fontId).at(0));
}

void View::playSounds()
{
    // (No rocket noise while waiting to start, even if thrust is held)
    if (m_model.getShipModel()->isAccelerating() && m_model.getGameState() != GameState::GetReady) {
        float vol = m_model.getShipModel()->accelerationAmount() * 2000.f;
        vol = std::clamp(vol, 2.f, 100.f);

        m_audio.soundLoop("rocket", vol);
    } else {
        m_audio.soundFade("rocket");
    }

    if (m_model.getGameState() == GameState::Exploding) {
        m_audio.soundPlay("collision");
    }
}

void View::stopSounds()
{
    m_audio.soundFade("rocket");
}

void View::draw(QPainter& painter, const QSizeF& canvasSize) const
{
    // TODO: this should take a model as its parameter; we shouldn't be bound
    // to one model but should be able to accept any model implementing a
    // specific interface. This would enable us to use models for splash
    // screens / high score screens etc.

    // This is where all state is "realised" as images drawn. (Sounds are handled
    // separately in playSounds(), which is called at the fixed simulation rate.)

    // Fill slightly beyond the canvas to avoid any rounding gap at the edges
    painter.fillRect(QRectF(0, 0, canvasSize.width() + 1, canvasSize.height() + 1), Qt::black);

    // Dynamic shapes (i.e. shapes which rotate around the ship)
    for (const auto& shape : m_model.getAllDynamicObjects()) {
        if (shape->isVisible() && shape->IsActive()) {
            rotateAndDrawShape(painter, canvasSize, *shape);
        }
    }

    // Draw black rectangle at top of screen for status bar (and hiding the "notch" on
    // macs in full screen)
    painter.fillRect(QRectF(0, 0, canvasSize.width() + 1, 75), Qt::black);

    // Lives remaining
    drawText(
        painter,
        canvasSize,
        "Ships remaining: " + std::to_string(m_model.getLivesRemaining()),
        QColor(0, 128, 0),
        40,
        30,
        20);

    double messageY = canvasSize.height() * 0.3;

    if (m_model.getGameState() == GameState::GetReady) {
        drawText(painter, canvasSize, "Get Ready", QColor(255, 255, 0), 160, {}, messageY);
        if (!m_model.levelDescription().empty()) {
            drawText(
                painter,
                canvasSize,
                m_model.levelDescription(),
                QColor(0, 192, 0),
                60,
                {},
                messageY + 200);
        }
        drawText(
            painter,
            canvasSize,
            "Press any key to start",
            QColor(160, 160, 160),
            40,
            {},
            messageY + 300);
    }

    if (m_model.gameIsPaused()) {
        drawText(painter, canvasSize, "Paused", QColor(255, 255, 0), 160, {}, messageY);
        // TODO do we need pause if opening the menu does the same thing?
    }

    if (m_model.getGameState() == GameState::Succeeded) {
        drawText(painter, canvasSize, "Success!", QColor(255, 255, 0), 160, {}, messageY);
    }

    if (m_model.getGameState() == GameState::Dead && m_model.getLivesRemaining() == 0) {
        drawText(painter, canvasSize, "Game Over", QColor(255, 0, 0), 160, {}, messageY);
    }
}

void View::rotateAndDrawShape(QPainter& painter, const QSizeF& canvasSize, const GameShape& shape)
    const
{
    // The viewport always shows the same area of the arena (480x300 units
    // worth), regardless of its physical dimensions, so a differently-shaped
    // window doesn't let the player see more of the maze:
    double width = canvasSize.width();
    double height = canvasSize.height();
    double scale = std::sqrt((width * height) / (480.0 * 300.0));
    double xOffset = width / 2.0;
    double yOffset = height / 2.0;

    for (const auto& sl : shape.getVec()) {
        double x0 = sl.x0 + shape.getPosX() - m_model.getShipModel()->x();
        double y0 = sl.y0 + shape.getPosY() - m_model.getShipModel()->y();
        double x1 = sl.x1 + shape.getPosX() - m_model.getShipModel()->x();
        double y1 = sl.y1 + shape.getPosY() - m_model.getShipModel()->y();
        // OK, when we get here, we have a line expressed
        // relative to the origin OF THE SHIP.
        double dCos = utils::cosine(m_model.getShipModel()->rotation());
        double dSin = utils::sine(m_model.getShipModel()->rotation());
        double x0r, y0r, x1r, y1r;
        x0r = x0 * dCos - y0 * dSin;
        y0r = x0 * dSin + y0 * dCos;
        x1r = x1 * dCos - y1 * dSin;
        y1r = x1 * dSin + y1 * dCos;
        // now draw adjusted for physical screen coords
        uint8_t r = sl.r;
        uint8_t g = sl.g;
        uint8_t b = sl.b;
        // Special case for objects with gravity, we add random colours
        if (shape.getGravity() != 0.f) {
            r = utils::rnd(193);
            g = utils::rnd(193);
            b = utils::rnd(193);
        }
        if (m_model.getGameState() == GameState::Menu
            || m_model.getGameState() == GameState::Paused) {
            // Dim everything
            r *= 0.4;
            g *= 0.4;
            b *= 0.4;
        }
        painter.setPen(QPen(
            QColor(r, g, b), sl.lineThickness * LINE_THICKNESS_SCALE, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QLineF(
            x0r * scale + xOffset,
            y0r * scale + yOffset,
            x1r * scale + xOffset,
            y1r * scale + yOffset));
    }
}

void View::drawText(
    QPainter& painter,
    const QSizeF& canvasSize,
    const std::string& text,
    const QColor& colour,
    int pixelSize,
    std::optional<double> x,
    std::optional<double> y) const
{
    QFont font(m_font);
    font.setPixelSize(pixelSize);
    QFontMetricsF metrics(font);
    QString string = QString::fromStdString(text);

    double left = x.has_value() ? x.value()
                                : (canvasSize.width() - metrics.horizontalAdvance(string)) / 2.0;
    double baseline = y.has_value()
        ? y.value() + metrics.ascent()
        : (canvasSize.height() + metrics.ascent() - metrics.descent()) / 2.0;

    painter.setFont(font);
    painter.setPen(colour);
    painter.drawText(QPointF(left, baseline), string);
}

} // namespace amaze
} // namespace marengo
