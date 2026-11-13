#include "view.h"
#include "audio.h"
#include "exceptions.h"
#include "shape.h"
#include "utils.h"
#include "viewtransform.h"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>

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
        throw AmazeRuntimeException("Font file load error");
    }
    m_font = QFont(QFontDatabase::applicationFontFamilies(fontId).at(0));
}

void View::playSounds()
{
    // (No rocket noise while waiting to start, even if thrust is held)
    if (m_model.shipModel().isAccelerating() && m_model.getGameState() != GameState::GetReady) {
        float vol = m_model.shipModel().accelerationAmount() * 2000.f;
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
    // This is where all state is "realised" as images drawn. (Sounds are handled
    // separately in playSounds(), which is called at the fixed simulation rate.)

    // Fill slightly beyond the canvas to avoid any rounding gap at the edges
    painter.fillRect(QRectF(0, 0, canvasSize.width() + 1, canvasSize.height() + 1), Qt::black);

    // Dynamic shapes (i.e. shapes which rotate around the ship): the view is
    // centred on the ship, and the arena rotates around it
    const auto& ship = m_model.shipModel();
    QTransform arenaTransform = arenaToCanvas(ship.x(), ship.y(), ship.rotation(), canvasSize);
    for (const auto& shape : m_model.getAllDynamicObjects()) {
        if (shape->isVisible() && shape->IsActive()) {
            drawShape(painter, arenaTransform, *shape);
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
    }

    if (m_model.getGameState() == GameState::Succeeded) {
        drawText(painter, canvasSize, "Success!", QColor(255, 255, 0), 160, {}, messageY);
    }

    if (m_model.getGameState() == GameState::Dead && m_model.getLivesRemaining() == 0) {
        drawText(painter, canvasSize, "Game Over", QColor(255, 0, 0), 160, {}, messageY);
    }
}

void View::drawShape(QPainter& painter, const QTransform& arenaTransform, const GameShape& shape)
    const
{
    // Shape lines are relative to the shape's position in the arena
    QTransform transform
        = QTransform::fromTranslate(shape.getPosX(), shape.getPosY()) * arenaTransform;

    for (const auto& sl : shape.getVec()) {
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
        // The pen is applied after mapping to the canvas, so line widths
        // aren't affected by the shape's or view's scaling
        painter.setPen(QPen(
            QColor(r, g, b), sl.lineThickness * LINE_THICKNESS_SCALE, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(transform.map(QLineF(sl.x0, sl.y0, sl.x1, sl.y1)));
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
