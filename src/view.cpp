#include "view.h"
#include "audio.h"
#include "exceptions.h"
#include "shape.h"
#include "utils.h"
#include "viewtransform.h"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QRadialGradient>
#include <QTextDocument>

#include <algorithm>
#include <cmath>
#include <format>

namespace marengo {
namespace amaze {

namespace {

// Line thicknesses in shapes are multiplied by this. (Their values were tuned
// on a 1920x1200 screen, whose height/1080 this is; the canvas always has
// the same area as that screen.)
constexpr double LINE_THICKNESS_SCALE = 1600.0 / 1080.0;

// How bright fuel is drawn, relative to its normal colour, while the shield is
// full and so it can't be collected
constexpr double FUEL_NOT_NEEDED_BRIGHTNESS = 0.2;

// The outer fraction of the vignette's radius over which the arena fades to black
constexpr double VIGNETTE_FADE_FRACTION = 0.2;

// Status panel, in the top left corner. Its labels, border and the
// bars' outlines are in STATUS_COLOUR.
const QColor STATUS_COLOUR(0, 128, 0);
// The filled part of the shield and thrust bars: darker than the outline, but
// brighter than the background grid
const QColor BAR_FILL_COLOUR(0, 80, 0);
// The shield bar's fill fades from BAR_FILL_COLOUR at full shield through
// yellow and orange to red at SHIELD_BAR_COLOUR_STOPS' lowest level and below,
// blending between neighbouring stops. The stops are about as dim as
// BAR_FILL_COLOUR, so the bar doesn't overpower the panel's text.
struct ShieldBarColourStop
{
    double level;
    QColor colour;
};
const ShieldBarColourStop SHIELD_BAR_COLOUR_STOPS[] = {
    { 0.2, QColor(110, 0, 0) },
    { 0.45, QColor(100, 50, 0) },
    { 0.7, QColor(85, 85, 0) },
    { 1.0, BAR_FILL_COLOUR },
};
const QColor PANEL_BACKGROUND_COLOUR(0, 14, 0);
// The inner bezel line and the dividers between sections
const QColor PANEL_BEZEL_COLOUR(0, 56, 0);
const QColor PANEL_VALUE_COLOUR(0, 210, 0);
const QColor PANEL_FAULT_COLOUR(210, 40, 40);
const QColor PANEL_DEPLETED_COLOUR(220, 150, 0);
constexpr double PANEL_LEFT = 30.0;
constexpr double PANEL_TOP = 30.0;
constexpr int PANEL_FONT_SIZE = 22;
constexpr int PANEL_TITLE_FONT_SIZE = 16;
constexpr double PANEL_PADDING = 18.0;
constexpr double PANEL_COLUMN_GAP = 30.0;
// Rows are this many times the text's height apart
constexpr double PANEL_ROW_SPACING = 1.45;
// Space above and below each divider
constexpr double PANEL_DIVIDER_SPACE = 8.0;
constexpr double PANEL_CORNER_RADIUS = 10.0;
constexpr double PANEL_BEZEL_INSET = 5.0;
// The view of the arena is moved right (if there's room) so that the
// vignette, out to where it's mostly dark, keeps this far clear of the panel
constexpr double PANEL_VIEW_CLEARANCE = 20.0;

// For the velocity readout, the ship (39 units from nose to the tips of its
// fins) is taken to be 10m long
constexpr double METRES_PER_UNIT = 10.0 / 39.0;
// The gravitational field is shown as a percentage of this pull (in units per
// tick per tick): the keyboard's full thrust, so 100% or more means it can't
// be escaped by thrusting directly away from it (except, perhaps, with a
// gamepad, whose full thrust is stronger)
constexpr double KEYBOARD_THRUST = 6.0 / 1000.0;
// The thrust bar is full at the strongest thrust: the gamepad's stick pushed
// all the way (see GameShell and Controller)
constexpr double GAMEPAD_FULL_THRUST = 15.0 / 1000.0;
// The thrust bar shows the thrust (as a fraction of the full thrust) raised to
// this power, so the low thrust used most of the time takes up more of the
// bar. Lower emphasises low thrust more; 1 would be linear.
constexpr double THRUST_BAR_EXPONENT = 0.5;
// The thrust bar moves towards the current thrust with this time constant
// (seconds): it covers about 63% of the gap in this time. Higher is smoother
// but slower to respond.
constexpr double THRUST_BAR_SMOOTHING_SECONDS = 0.1;
constexpr double MAX_GRAVITY_PERCENT = 999.0;

// Debris falling into gravity objects: a little brighter than the background
// grid (see GameModel::initialise()), fading in as it appears
const QColor GRAVITY_DEBRIS_COLOUR(0, 72, 0);
constexpr double GRAVITY_DEBRIS_SIZE = 5.0;
constexpr int GRAVITY_DEBRIS_FADE_IN_TICKS = 60;

// The level's introduction (see drawIntroduction()). The panel is at most
// INTRO_MAX_WIDTH wide, keeping INTRO_MARGIN clear of the canvas's edges and
// the status panel, with its top INTRO_TOP of the way down the canvas.
constexpr double INTRO_MAX_WIDTH = 1100.0;
constexpr double INTRO_MARGIN = 40.0;
constexpr double INTRO_TOP = 0.15;
constexpr double INTRO_PADDING = 36.0;
constexpr int INTRO_TITLE_FONT_SIZE = 60;
constexpr int INTRO_FONT_SIZE = 32;
const QColor INTRO_TITLE_COLOUR(0, 192, 0);
const QColor INTRO_TEXT_COLOUR(200, 220, 200);
// The panel is a little translucent, so the level shows faintly through it
constexpr int INTRO_BACKGROUND_ALPHA = 230;

// The ship's systems, which are shown but never change
struct SystemStatus {
    const char* label;
    const char* value;
    const QColor& colour;
};
const SystemStatus SYSTEMS[] = {
    { "Retrothrusters:", "MALFUNCTION", PANEL_FAULT_COLOUR },
    { "Long-range scanner:", "MALFUNCTION", PANEL_FAULT_COLOUR },
    { "Weapons:", "DEPLETED", PANEL_DEPLETED_COLOUR },
    { "Escape pod:", "MALFUNCTION", PANEL_FAULT_COLOUR },
};
const char* const READING_LABELS[]
    = { "Shield:", "Thrust:", "Velocity:", "Bearing:", "Gravitational field:" };
// The widest values each reading is expected to show, so the panel doesn't
// change size
const char* const WIDEST_READINGS[] = { "888.8 m/s", "888°", "888%" };

QFont withPixelSize(QFont font, int pixelSize)
{
    font.setPixelSize(pixelSize);
    return font;
}

// The shield bar's fill colour for a shield level from 0 to 1
QColor shieldBarColour(double shield)
{
    const auto& stops = SHIELD_BAR_COLOUR_STOPS;
    if (shield <= stops[0].level) {
        return stops[0].colour;
    }
    for (size_t i = 1; i < std::size(stops); ++i) {
        const auto& lower = stops[i - 1];
        const auto& upper = stops[i];
        if (shield <= upper.level) {
            double t = (shield - lower.level) / (upper.level - lower.level);
            auto mix = [t](int a, int b) { return static_cast<int>(std::lround(a + (b - a) * t)); };
            return QColor(
                mix(lower.colour.red(), upper.colour.red()),
                mix(lower.colour.green(), upper.colour.green()),
                mix(lower.colour.blue(), upper.colour.blue()));
        }
    }
    return std::end(stops)[-1].colour;
}

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

    // The status panel's layout doesn't depend on the canvas's size, so is
    // worked out here
    QFontMetricsF metrics(withPixelSize(m_font, PANEL_FONT_SIZE));
    QFontMetricsF titleMetrics(withPixelSize(m_font, PANEL_TITLE_FONT_SIZE));
    double labelWidth = 0.0;
    double valueWidth = 0.0;
    for (const char* label : READING_LABELS) {
        labelWidth = std::max(labelWidth, metrics.horizontalAdvance(QString::fromUtf8(label)));
    }
    for (const char* value : WIDEST_READINGS) {
        valueWidth = std::max(valueWidth, metrics.horizontalAdvance(QString::fromUtf8(value)));
    }
    for (const auto& system : SYSTEMS) {
        labelWidth
            = std::max(labelWidth, metrics.horizontalAdvance(QString::fromUtf8(system.label)));
        valueWidth
            = std::max(valueWidth, metrics.horizontalAdvance(QString::fromUtf8(system.value)));
    }
    m_panelRowHeight = metrics.height() * PANEL_ROW_SPACING;
    m_panelTitleHeight = titleMetrics.height() * PANEL_ROW_SPACING;
    m_panelValueWidth = valueWidth;
    double rows = std::size(READING_LABELS) + std::size(SYSTEMS);
    m_panelRect = QRectF(
        PANEL_LEFT,
        PANEL_TOP,
        PANEL_PADDING * 2 + labelWidth + PANEL_COLUMN_GAP + valueWidth,
        PANEL_PADDING * 2 + m_panelTitleHeight + rows * m_panelRowHeight
            + 2 * (2 * PANEL_DIVIDER_SPACE));
}

View::~View() = default;

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
    QPointF centre = viewCentre(canvasSize);
    QTransform arenaTransform
        = arenaToCanvas(ship.x(), ship.y(), ship.rotation(), canvasSize, centre);
    for (const auto& shape : m_model.getAllDynamicObjects()) {
        if (shape->isVisible() && shape->IsActive()) {
            drawShape(painter, arenaTransform, *shape);
        }
        // Gravity fields go just above the background grid, so under
        // decorations and the ship
        if (shape->getName() == "Background") {
            drawGravityFields(painter, arenaTransform);
        }
    }

    // Vignette: fade the arena to black around the ship, reaching full black on
    // a circle whose diameter is the canvas's shorter side, so turning the ship
    // doesn't let the player see further along the long axis
    double vignetteRadius = std::min(canvasSize.width(), canvasSize.height()) / 2.0;
    QRadialGradient vignette(centre, vignetteRadius);
    vignette.setColorAt(0.0, QColor(0, 0, 0, 0));
    vignette.setColorAt(1.0 - VIGNETTE_FADE_FRACTION, QColor(0, 0, 0, 0));
    vignette.setColorAt(1.0, QColor(0, 0, 0, 255)); // and beyond, by the default pad spread
    painter.fillRect(QRectF(0, 0, canvasSize.width() + 1, canvasSize.height() + 1), vignette);

    // Black band at the top of the screen, hiding the "notch" on macs in full
    // screen
    painter.fillRect(QRectF(0, 0, canvasSize.width() + 1, 75), Qt::black);

    drawStatusPanel(painter);

    double messageY = canvasSize.height() * 0.3;

    if (m_model.getGameState() == GameState::GetReady) {
        drawGetReady(painter, centre, canvasSize);
    }

    if (m_model.gameIsPaused()) {
        drawText(painter, centre, "Paused", QColor(255, 255, 0), 160, {}, messageY);
    }

    if (m_model.getGameState() == GameState::Succeeded) {
        drawText(painter, centre, "Success!", QColor(255, 255, 0), 160, {}, messageY);
    }

    if (m_model.getGameState() == GameState::Dead) {
        drawText(painter, centre, "Game Over", QColor(255, 0, 0), 160, {}, messageY);
    }
}

void View::drawGetReady(QPainter& painter, const QPointF& centre, const QSizeF& canvasSize) const
{
    double messageY = canvasSize.height() * 0.3;
    double promptY = messageY + 300;
    if (m_model.levelIntroduction().empty()) {
        drawText(painter, centre, "Get Ready", QColor(255, 255, 0), 160, {}, messageY);
        if (!m_model.levelDescription().empty()) {
            drawText(
                painter,
                centre,
                m_model.levelDescription(),
                INTRO_TITLE_COLOUR,
                60,
                {},
                messageY + 200);
        }
    } else {
        QRectF panel = drawIntroduction(painter, centre, canvasSize);
        promptY = panel.bottom() + INTRO_MARGIN;
    }
    drawText(
        painter, centre, "Press any key to start", QColor(160, 160, 160), 40, {}, promptY);
}

QRectF
View::drawIntroduction(QPainter& painter, const QPointF& centre, const QSizeF& canvasSize) const
{
    // Centred on the ship (like the other messages), as wide as there's room
    // for between the status panel and the right of the canvas
    double halfWidth = std::min(
        { INTRO_MAX_WIDTH / 2.0,
          centre.x() - m_panelRect.right() - INTRO_MARGIN,
          canvasSize.width() - INTRO_MARGIN - centre.x() });
    double textWidth = std::max(halfWidth * 2.0 - INTRO_PADDING * 2.0, 100.0);

    const std::string& source = m_model.levelIntroduction();
    if (!m_introduction || source != m_introductionSource) {
        m_introductionSource = source;
        m_introduction = std::make_unique<QTextDocument>();
        m_introduction->setDocumentMargin(0);
        m_introduction->setDefaultFont(withPixelSize(m_font, INTRO_FONT_SIZE));
        QTextOption option;
        option.setAlignment(Qt::AlignHCenter);
        option.setWrapMode(QTextOption::WordWrap);
        m_introduction->setDefaultTextOption(option);
        // HTML if it starts with a tag (see LEVEL_FORMAT.md), else plain text
        QString text = QString::fromStdString(source);
        if (text.trimmed().startsWith('<')) {
            m_introduction->setHtml(text);
        } else {
            m_introduction->setPlainText(text);
        }
    }
    if (m_introduction->textWidth() != textWidth) {
        m_introduction->setTextWidth(textWidth);
    }

    QFont titleFont = withPixelSize(m_font, INTRO_TITLE_FONT_SIZE);
    QFontMetricsF titleMetrics(titleFont);
    const std::string& title = m_model.levelDescription();
    double titleHeight = title.empty() ? 0.0 : titleMetrics.height() + 2 * PANEL_DIVIDER_SPACE;

    QRectF panel(
        centre.x() - textWidth / 2.0 - INTRO_PADDING,
        canvasSize.height() * INTRO_TOP,
        textWidth + INTRO_PADDING * 2.0,
        INTRO_PADDING * 2.0 + titleHeight + m_introduction->size().height());
    QColor background = PANEL_BACKGROUND_COLOUR;
    background.setAlpha(INTRO_BACKGROUND_ALPHA);
    drawPanelBox(painter, panel, background);

    double y = panel.top() + INTRO_PADDING;
    if (!title.empty()) {
        drawText(painter, centre, title, INTRO_TITLE_COLOUR, INTRO_TITLE_FONT_SIZE, {}, y);
        y += titleMetrics.height() + PANEL_DIVIDER_SPACE;
        painter.setPen(QPen(PANEL_BEZEL_COLOUR, 1));
        painter.drawLine(
            QPointF(panel.left() + INTRO_PADDING, y), QPointF(panel.right() - INTRO_PADDING, y));
        y += PANEL_DIVIDER_SPACE;
    }

    // Text without a colour of its own (from the HTML) is in INTRO_TEXT_COLOUR
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, INTRO_TEXT_COLOUR);
    painter.save();
    painter.translate(panel.left() + INTRO_PADDING, y);
    m_introduction->documentLayout()->draw(&painter, context);
    painter.restore();
    return panel;
}

void View::drawPanelBox(QPainter& painter, const QRectF& rect, const QColor& background) const
{
    // The box, with a bezel inside its border
    painter.setPen(QPen(STATUS_COLOUR, 2));
    painter.setBrush(background);
    painter.drawRoundedRect(rect, PANEL_CORNER_RADIUS, PANEL_CORNER_RADIUS);
    painter.setPen(QPen(PANEL_BEZEL_COLOUR, 1));
    painter.setBrush(Qt::NoBrush);
    double inset = PANEL_BEZEL_INSET;
    painter.drawRoundedRect(
        rect.adjusted(inset, inset, -inset, -inset),
        PANEL_CORNER_RADIUS - inset,
        PANEL_CORNER_RADIUS - inset);
}

QPointF View::viewCentre(const QSizeF& canvasSize) const
{
    // The middle of the canvas, unless the vignette would be too close to the
    // status panel, when it's moved right to clear it, but not so far that the
    // vignette goes off the right of the canvas
    double radius = std::min(canvasSize.width(), canvasSize.height()) / 2.0;
    QPointF centre(canvasSize.width() / 2.0, canvasSize.height() / 2.0);
    double clearRadius = radius * (1.0 - VIGNETTE_FADE_FRACTION / 2.0) + PANEL_VIEW_CLEARANCE;
    // The panel's nearest point to the centre
    double dy = std::max(0.0, centre.y() - m_panelRect.bottom());
    if (dy < clearRadius) {
        double clearX = m_panelRect.right() + std::sqrt(clearRadius * clearRadius - dy * dy);
        double maxX = canvasSize.width() - radius;
        centre.setX(std::max(centre.x(), std::min(clearX, maxX)));
    }
    return centre;
}

void View::drawStatusPanel(QPainter& painter) const
{
    QFont font = withPixelSize(m_font, PANEL_FONT_SIZE);
    QFont titleFont = withPixelSize(m_font, PANEL_TITLE_FONT_SIZE);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, 130);
    QFontMetricsF metrics(font);
    QFontMetricsF titleMetrics(titleFont);

    drawPanelBox(painter, m_panelRect, PANEL_BACKGROUND_COLOUR);

    double left = m_panelRect.left() + PANEL_PADDING;
    double right = m_panelRect.right() - PANEL_PADDING;
    double y = m_panelRect.top() + PANEL_PADDING;

    // Each row's text is vertically centred in the row
    auto baseline = [](double rowTop, double rowHeight, const QFontMetricsF& m) {
        return rowTop + (rowHeight + m.ascent() - m.descent()) / 2.0;
    };
    auto divider = [&]() {
        y += PANEL_DIVIDER_SPACE;
        painter.setPen(QPen(PANEL_BEZEL_COLOUR, 1));
        painter.drawLine(QPointF(left, y), QPointF(right, y));
        y += PANEL_DIVIDER_SPACE;
    };
    // A row with a label on the left and, if given, a value on the right.
    // Returns the row's area.
    auto row = [&](const char* label, const QString& value, const QColor& valueColour) {
        QRectF area(left, y, right - left, m_panelRowHeight);
        double textY = baseline(y, m_panelRowHeight, metrics);
        painter.setFont(font);
        painter.setPen(STATUS_COLOUR);
        painter.drawText(QPointF(left, textY), QString::fromUtf8(label));
        if (!value.isEmpty()) {
            painter.setPen(valueColour);
            painter.drawText(QPointF(right - metrics.horizontalAdvance(value), textY), value);
        }
        y += m_panelRowHeight;
        return area;
    };

    QString title = "SHIP STATUS";
    painter.setFont(titleFont);
    painter.setPen(STATUS_COLOUR);
    painter.drawText(
        QPointF(
            m_panelRect.center().x() - titleMetrics.horizontalAdvance(title) / 2.0,
            baseline(y, m_panelTitleHeight, titleMetrics)),
        title);
    y += m_panelTitleHeight;
    divider();

    // A row with a bar, filled to `fraction` (0 to 1) of its width
    auto barRow = [&](const char* label, double fraction, const QColor& fillColour) {
        QRectF area = row(label, {}, PANEL_VALUE_COLOUR);
        double barHeight = metrics.height() * 0.6;
        QRectF bar(
            right - m_panelValueWidth,
            area.center().y() - barHeight / 2.0,
            m_panelValueWidth,
            barHeight);
        QRectF filled(bar);
        filled.setWidth(bar.width() * std::clamp(fraction, 0.0, 1.0));
        painter.fillRect(filled, fillColour);
        painter.setPen(QPen(STATUS_COLOUR, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(bar);
    };

    const auto& ship = m_model.shipModel();
    // Shield, which empties as the shield weakens
    barRow(READING_LABELS[0], m_model.shield(), shieldBarColour(m_model.shield()));
    double thrust = ship.isAccelerating() ? ship.accelerationAmount() / GAMEPAD_FULL_THRUST : 0.0;
    double target = std::pow(std::clamp(thrust, 0.0, 1.0), THRUST_BAR_EXPONENT);
    auto now = std::chrono::steady_clock::now();
    if (m_lastPanelDrawTime.has_value()) {
        double elapsed = std::chrono::duration<double>(now - *m_lastPanelDrawTime).count();
        // (Frame-rate independent: a long gap between frames moves the bar
        // most of the way at once)
        double step = 1.0 - std::exp(-elapsed / THRUST_BAR_SMOOTHING_SECONDS);
        m_thrustBarFill += (target - m_thrustBarFill) * step;
    } else {
        m_thrustBarFill = target;
    }
    m_lastPanelDrawTime = now;
    barRow(READING_LABELS[1], m_thrustBarFill, BAR_FILL_COLOUR);

    // Units per tick to metres per second
    double speed = std::hypot(ship.dX(), ship.dY()) * TICKS_PER_SECOND * METRES_PER_UNIT;
    row(READING_LABELS[2],
        QString::fromStdString(std::format("{:.1f} m/s", speed)),
        PANEL_VALUE_COLOUR);

    // Compass bearing of the ship's nose: clockwise from the arena's "up". The
    // ship's rotation is anticlockwise, and may be negative.
    int bearing = static_cast<int>(std::lround(std::fmod(360.0 - ship.rotation(), 360.0))) % 360;
    row(READING_LABELS[3],
        QString::fromStdString(std::format("{:03d}°", bearing)),
        PANEL_VALUE_COLOUR);

    double gravityPercent
        = std::min(m_model.gravitationalPull() / KEYBOARD_THRUST * 100.0, MAX_GRAVITY_PERCENT);
    row(READING_LABELS[4],
        QString::fromStdString(std::format("{:.0f}%", gravityPercent)),
        PANEL_VALUE_COLOUR);
    divider();

    for (const auto& system : SYSTEMS) {
        row(system.label, QString::fromUtf8(system.value), system.colour);
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
        // Fuel can't be collected while the shield is full (see
        // GameModel::collisionDetect()), so it's shown darker until it's needed
        if (shape.getGameShapeType() == GameShapeType::FUEL && m_model.shield() >= 1.0) {
            r *= FUEL_NOT_NEEDED_BRIGHTNESS;
            g *= FUEL_NOT_NEEDED_BRIGHTNESS;
            b *= FUEL_NOT_NEEDED_BRIGHTNESS;
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

void View::drawGravityFields(QPainter& painter, const QTransform& arenaTransform) const
{
    bool dimmed
        = m_model.getGameState() == GameState::Menu || m_model.getGameState() == GameState::Paused;
    for (const auto& field : m_model.gravityFields()) {
        const auto& shape = *field.shape;
        if (!shape.isVisible() || !shape.IsActive()) {
            continue;
        }
        QTransform transform
            = QTransform::fromTranslate(shape.getPosX(), shape.getPosY()) * arenaTransform;
        for (const auto& debris : field.debris) {
            QColor colour = GRAVITY_DEBRIS_COLOUR;
            if (dimmed) {
                colour = QColor(colour.red() * 0.4, colour.green() * 0.4, colour.blue() * 0.4);
            }
            colour.setAlphaF(
                std::min(static_cast<float>(debris.age) / GRAVITY_DEBRIS_FADE_IN_TICKS, 1.f));
            painter.setPen(QPen(
                colour, GRAVITY_DEBRIS_SIZE * LINE_THICKNESS_SCALE, Qt::SolidLine, Qt::RoundCap));
            painter.drawPoint(transform.map(debris.pos));
        }
    }
}

QRectF View::drawText(
    QPainter& painter,
    const QPointF& centre,
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

    double left = x.has_value() ? x.value() : centre.x() - metrics.horizontalAdvance(string) / 2.0;
    double baseline = y.has_value() ? y.value() + metrics.ascent()
                                    : centre.y() + (metrics.ascent() - metrics.descent()) / 2.0;

    painter.setFont(font);
    painter.setPen(colour);
    painter.drawText(QPointF(left, baseline), string);
    return QRectF(
        left,
        baseline - metrics.ascent(),
        metrics.horizontalAdvance(string),
        metrics.ascent() + metrics.descent());
}

} // namespace amaze
} // namespace marengo
