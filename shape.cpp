#include "shape.h"
#include "log.h"
#include "utils.h"
#include "vectorfont.h"

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
        std::vector<int> vertices = vectorfont::getCharacter(c);
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
    m_minX = 0;
    m_minY = 0;
    m_maxX = 0;
    m_maxY = 0;
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

bool Shape::intersectCheck(std::shared_ptr<Shape> s) const
{
    // check for intersect with all of my lines against all of another object's:
    for (size_t myLine = 0; myLine < m_shapeLines.size(); ++myLine) {
        double x0 = m_shapeLines[myLine].x0 + m_x;
        double y0 = m_shapeLines[myLine].y0 + m_y;
        double x1 = m_shapeLines[myLine].x1 + m_x;
        double y1 = m_shapeLines[myLine].y1 + m_y;
        for (unsigned int theirLine = 0; theirLine < s->getVec().size(); ++theirLine) {
            double x2 = s->getVec()[theirLine].x0 + s->getPosX();
            double y2 = s->getVec()[theirLine].y0 + s->getPosY();
            double x3 = s->getVec()[theirLine].x1 + s->getPosX();
            double y3 = s->getVec()[theirLine].y1 + s->getPosY();

            double ax, bx, cx, ay, by, cy, d, e, f;
            short x1lo, x1hi, y1lo, y1hi;

            ax = x1 - x0;
            bx = x2 - x3;

            // X bound box test:
            if (ax < 0) {
                x1lo = (short)x1;
                x1hi = (short)x0;
            } else {
                x1hi = (short)x1;
                x1lo = (short)x0;
            }
            if (bx > 0) {
                if (x1hi < (short)x3 || (short)x2 < x1lo) {
                    continue;
                }
            } else {
                if (x1hi < (short)x2 || (short)x3 < x1lo) {
                    continue;
                }
            }

            ay = y1 - y0;
            by = y2 - y3;

            // Y bound box test
            if (ay < 0.0) {
                y1lo = (short)y1;
                y1hi = (short)y0;
            } else {
                y1hi = (short)y1;
                y1lo = (short)y0;
            }
            if (by > 0.0) {
                if (y1hi < (short)y3 || (short)y2 < y1lo) {
                    continue;
                }
            } else {
                if (y1hi < (short)y2 || (short)y3 < y1lo) {
                    continue;
                }
            }

            cx = x0 - x2;
            cy = y0 - y2;
            d = by * cx - bx * cy; // alpha numerator
            f = ay * bx - ax * by; // both denominator
            // alpha tests
            if (f > 0.0) {
                if (d < 0.0 || d > f) {
                    continue;
                }
            } else {
                if (d > 0.0 || d < f) {
                    continue;
                }
            }

            e = ax * cy - ay * cx; // beta numerator
            // beta tests
            if (f > 0.0) {
                if (e < 0.0 || e > f) {
                    continue;
                }
            } else {
                if (e > 0.0 || e < f) {
                    continue;
                }
            }

            // if we get here, the lines either intersect or are collinear.
            return true;
        }
    }
    // if we get here we had no intersects
    return false;
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
