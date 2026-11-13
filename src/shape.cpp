#include "shape.h"
#include "log.h"
#include "utils.h"
#include "vectorfont.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <format>
#include <sstream>
#include <string>
#include <vector>

namespace marengo {
namespace amaze {

Shape::Shape() { }

Shape::~Shape() { }

void Shape::makeFromText(
    const std::string& s,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a,
    int lineThickness)
{
    m_shapeLines.clear();
    double textWidth = 0;
    double prevX = -1;
    double prevY = -1;

    for (const char& c : s) {
        double characterWidth = 0.0;
        const std::vector<int>& vertices = vectorfont::getCharacter(c);
        size_t numVertices = vertices.size() / 2;
        if (numVertices == 0) {
            // Empty cell, presumably a space
            characterWidth = 10;
        }
        for (unsigned int j = 0; j < numVertices; ++j) {
            double x = vertices[j * 2];
            double y = vertices[j * 2 + 1];
            if (prevX != -1 && prevY != -1 && x != -1 && y != -1) {
                ShapeLine sl { prevX, prevY, x + textWidth, y, r, g, b, a, lineThickness };
                addShapeLine(sl);
            }
            prevX = x + textWidth;
            prevY = y;
            if (characterWidth < x) {
                characterWidth = x;
            }
        }
        // new character:
        prevX = -1;
        prevY = -1;
        textWidth += characterWidth + 2; // add a bit of extra spacing between characters
    }

    // Finally a bit of post-processing: the centre point of this shape will
    // not be correct (as we've built it L->R), so modify it so it is centred:
    for (ShapeLine& sl : m_shapeLines) {
        sl.x0 -= (m_width / 2);
        sl.y0 -= (m_height / 2);
        sl.x1 -= (m_width / 2);
        sl.y1 -= (m_height / 2);
    }
}

void Shape::addShapeLine(ShapeLine sl)
{
    updateShapeSize(sl.x0, sl.y0, sl.x1, sl.y1);
    m_shapeLines.push_back(sl);
}

void Shape::setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    m_r = r;
    m_g = g;
    m_b = b;
    m_a = a;
}

void Shape::addLine(double x0, double y0, double x1, double y1, uint8_t thickness)
{
    addShapeLine(ShapeLine { x0, y0, x1, y1, m_r, m_g, m_b, m_a, thickness });
}

double Shape::getWidth() const
{
    return m_width;
}

double Shape::getHeight() const
{
    return m_height;
}

void Shape::setPos(double x, double y)
{
    m_x = x;
    m_y = y;
}

double Shape::getPosX() const
{
    return m_x;
}

double Shape::getPosY() const
{
    return m_y;
}

bool Shape::isVisible() const
{
    return m_isVisible;
}

void Shape::setVisible(bool b)
{
    m_isVisible = b;
}

double Shape::getScale() const
{
    return m_scale;
}

void Shape::rotate(double rotationDelta)
{
    // This is not related to the rotation of the entire "world" around
    // the ship - this will simply cause a shape to rotate around its centre
    // (Note - shapes' lines should be defined relative to this centre
    if (rotationDelta == 0.0) {
        return;
    }

    std::vector<ShapeLine> tmp;
    tmp.reserve(m_shapeLines.size());

    double rd = std::fmod(rotationDelta, 360.0);

    for (ShapeLine line : m_shapeLines) {
        double x0r = line.x0 * utils::cosine(rd) - line.y0 * utils::sine(rd);
        double y0r = line.x0 * utils::sine(rd) + line.y0 * utils::cosine(rd);
        double x1r = line.x1 * utils::cosine(rd) - line.y1 * utils::sine(rd);
        double y1r = line.x1 * utils::sine(rd) + line.y1 * utils::cosine(rd);

        line.x0 = x0r;
        line.y0 = y0r;
        line.x1 = x1r;
        line.y1 = y1r;
        tmp.push_back(line);
    }
    std::swap(tmp, m_shapeLines);
}

void Shape::clear()
{
    m_shapeLines.clear();
    m_width = 0;
    m_height = 0;
    m_minX = std::numeric_limits<double>::max();
    m_minY = std::numeric_limits<double>::max();
    m_maxX = std::numeric_limits<double>::lowest();
    m_maxY = std::numeric_limits<double>::lowest();
    m_rotation = 0;
}

void Shape::resize(double scale)
{
    if (scale > 0.0) {
        m_scale = scale;
    }
    for (auto& sl : m_shapeLines) {
        sl.x0 *= scale;
        sl.y0 *= scale;
        sl.x1 *= scale;
        sl.y1 *= scale;
    }
    m_width = static_cast<int>(static_cast<double>(m_width) * m_scale);
    m_height = static_cast<int>(static_cast<double>(m_height) * m_scale);
}

namespace {

// Is point p on the line segment? (Exact test: only used for the collinear
// case, where lines are e.g. both horizontal or vertical.)
bool pointOnSegment(const QPointF& p, const QLineF& segment)
{
    QPointF direction = segment.p2() - segment.p1();
    QPointF offset = p - segment.p1();
    if (direction.isNull()) {
        return offset.isNull(); // zero-length segment, i.e. a point
    }
    if (direction.x() * offset.y() - direction.y() * offset.x() != 0.0) {
        return false; // not on the (infinite) line
    }
    double along = QPointF::dotProduct(offset, direction);
    return along >= 0.0 && along <= QPointF::dotProduct(direction, direction);
}

bool linesTouch(const QLineF& a, const QLineF& b)
{
    if (a.intersects(b) == QLineF::BoundedIntersection) {
        return true;
    }
    // QLineF treats parallel lines (including collinear ones, and zero-length
    // lines, i.e. points) as never intersecting. But collinear lines which
    // overlap, or a point on a line, should count as touching. In those cases
    // at least one end point lies on the other line.
    return pointOnSegment(a.p1(), b) || pointOnSegment(a.p2(), b) || pointOnSegment(b.p1(), a)
        || pointOnSegment(b.p2(), a);
}

} // namespace

bool Shape::intersectCheck(std::shared_ptr<Shape> s) const
{
    // check for intersect with all of my lines against all of another object's:
    for (const auto& mine : m_shapeLines) {
        QLineF myLine(mine.x0 + m_x, mine.y0 + m_y, mine.x1 + m_x, mine.y1 + m_y);
        for (const auto& theirs : s->getVec()) {
            QLineF theirLine(
                theirs.x0 + s->getPosX(),
                theirs.y0 + s->getPosY(),
                theirs.x1 + s->getPosX(),
                theirs.y1 + s->getPosY());
            if (linesTouch(myLine, theirLine)) {
                return true;
            }
        }
    }
    return false;
}

std::optional<QPointF> Shape::contactNormal(const std::shared_ptr<Shape>& other) const
{
    QPointF centre(m_x, m_y);
    QPointF sum;
    bool touching = false;
    for (const auto& theirs : other->getVec()) {
        QLineF theirLine(
            theirs.x0 + other->getPosX(),
            theirs.y0 + other->getPosY(),
            theirs.x1 + other->getPosX(),
            theirs.y1 + other->getPosY());
        bool touchesThisLine = false;
        for (const auto& mine : m_shapeLines) {
            QLineF myLine(mine.x0 + m_x, mine.y0 + m_y, mine.x1 + m_x, mine.y1 + m_y);
            if (linesTouch(myLine, theirLine)) {
                touchesThisLine = true;
                break;
            }
        }
        if (!touchesThisLine) {
            continue;
        }
        touching = true;
        // Direction from the closest point on their line to our centre. For a
        // hit along a line this is perpendicular to it; for a hit on the end
        // of a line (e.g. a corner) it points away from that end.
        QPointF direction = theirLine.p2() - theirLine.p1();
        double lengthSquared = QPointF::dotProduct(direction, direction);
        double along = 0.0;
        if (lengthSquared > 0.0) {
            along = std::clamp(
                QPointF::dotProduct(centre - theirLine.p1(), direction) / lengthSquared, 0.0, 1.0);
        }
        QPointF away = centre - (theirLine.p1() + direction * along);
        double distance = std::hypot(away.x(), away.y());
        if (distance > 0.0) {
            sum += away / distance;
        }
    }
    double length = std::hypot(sum.x(), sum.y());
    if (!touching || length < 1e-9) {
        return std::nullopt;
    }
    return sum / length;
}

const std::vector<ShapeLine>& Shape::getVec() const
{
    return m_shapeLines;
}

void Shape::setPosFromCentre()
{
    // Convert all lines to be relative to the centre of the shape
    auto centreX = m_minX + (m_width / 2);
    auto centreY = m_minY + (m_height / 2);
    setPos(centreX, centreY);
    for (auto& sl : m_shapeLines) {
        sl.x0 -= centreX;
        sl.y0 -= centreY;
        sl.x1 -= centreX;
        sl.y1 -= centreY;
    }
}

void Shape::updateShapeSize(double x0, double y0, double x1, double y1)
{
    // update the size of the shape:
    if (x0 < m_minX) {
        m_minX = x0;
    }
    if (x0 > m_maxX) {
        m_maxX = x0;
    }
    if (x1 < m_minX) {
        m_minX = x1;
    }
    if (x1 > m_maxX) {
        m_maxX = x1;
    }
    if (y0 < m_minY) {
        m_minY = y0;
    }
    if (y0 > m_maxY) {
        m_maxY = y0;
    }
    if (y1 < m_minY) {
        m_minY = y1;
    }
    if (y1 > m_maxY) {
        m_maxY = y1;
    }

    m_width = m_maxX - m_minX;
    m_height = m_maxY - m_minY;
}

} // namespace amaze
} // namespace marengo
