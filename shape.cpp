#include "shape.h"
#include "helperfunctions.h"
#include "log.h"
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
    m_ShapeLines.clear();
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
                ShapeLine sl { prevX, prevY, x + textWidth, y, r, g, b, a, lineThickness,
                               prevX, prevY, x + textWidth, y };
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
    for (ShapeLine& sl : m_ShapeLines) {
        sl.x0 -= (m_Width / 2);
        sl.y0 -= (m_Height / 2);
        sl.x1 -= (m_Width / 2);
        sl.y1 -= (m_Height / 2);
    }
}

void Shape::addShapeLine(ShapeLine sl)
{
    updateShapeSize(sl.x0, sl.y0, sl.x1, sl.y1);
    m_ShapeLines.push_back(sl);
}

void Shape::setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    m_R = r;
    m_G = g;
    m_B = b;
    m_A = a;
}

void Shape::addLine(double x0, double y0, double x1, double y1, uint8_t thickness)
{
    addShapeLine(ShapeLine { x0, y0, x1, y1, m_R, m_G, m_B, m_A, thickness, x0, y0, x1, y1 });
}

double Shape::getWidth() const
{
    return m_Width;
}

double Shape::getHeight() const
{
    return m_Height;
}

void Shape::setPos(double x, double y)
{
    m_X = x;
    m_Y = y;
}

double Shape::getPosX() const
{
    return m_X;
}

double Shape::getPosY() const
{
    return m_Y;
}

bool Shape::isVisible() const
{
    return m_IsVisible;
}

void Shape::setVisible(bool b)
{
    m_IsVisible = b;
}

double Shape::getScale() const
{
    return m_Scale;
}

void Shape::rotate(double rotationDelta)
{
    // This is not related to the rotation of the entire "world" around
    // the ship - this will simply cause a shape to rotate around its centre
    // (Note - shapes' lines should be defined relative to this centre - but
    // see setPosFromCentre() )
    if (rotationDelta == 0.0) {
        return;
    }

    std::vector<ShapeLine> tmp;
    tmp.reserve(m_ShapeLines.size());

    double rd = std::fmod(rotationDelta, 360.0);

    for (ShapeLine line : m_ShapeLines) {
        // adjust according to rotation
        double x0r = line.x0 * helperfunctions::cosine(rd) - line.y0 * helperfunctions::sine(rd);
        double y0r = line.x0 * helperfunctions::sine(rd) + line.y0 * helperfunctions::cosine(rd);
        double x1r = line.x1 * helperfunctions::cosine(rd) - line.y1 * helperfunctions::sine(rd);
        double y1r = line.x1 * helperfunctions::sine(rd) + line.y1 * helperfunctions::cosine(rd);

        line.x0 = x0r;
        line.y0 = y0r;
        line.x1 = x1r;
        line.y1 = y1r;
        tmp.push_back(line);
    }
    std::swap(tmp, m_ShapeLines);
}

void Shape::clear()
{
    m_ShapeLines.clear();
    m_Width = 0;
    m_Height = 0;
    m_MinX = 0;
    m_MinY = 0;
    m_MaxX = 0;
    m_MaxY = 0;
    m_Rotation = 0;
}

void Shape::resize(double scale)
{
    if (scale > 0) {
        m_Scale = scale;
    }
    for (auto& sl : m_ShapeLines) {
        sl.x0 *= scale;
        sl.y0 *= scale;
        sl.x1 *= scale;
        sl.y1 *= scale;
    }
    m_Width = static_cast<int>(static_cast<double>(m_Width) * m_Scale);
    m_Height = static_cast<int>(static_cast<double>(m_Height) * m_Scale);
}

bool Shape::intersectCheck(std::shared_ptr<Shape> s) const
{
    // check for intersect with all of my lines against all of another object's:
    for (size_t myLine = 0; myLine < m_ShapeLines.size(); ++myLine) {
        double x0 = m_ShapeLines[myLine].x0 + m_X;
        double y0 = m_ShapeLines[myLine].y0 + m_Y;
        double x1 = m_ShapeLines[myLine].x1 + m_X;
        double y1 = m_ShapeLines[myLine].y1 + m_Y;
        for (unsigned int theirLine = 0; theirLine < s->getVec().size(); ++theirLine) {
            double x2 = s->getVec()[theirLine].x0 + s->getPosX();
            double y2 = s->getVec()[theirLine].y0 + s->getPosY();
            double x3 = s->getVec()[theirLine].x1 + s->getPosX();
            double y3 = s->getVec()[theirLine].y1 + s->getPosY();

            double Ax, Bx, Cx, Ay, By, Cy, d, e, f;
            short x1lo, x1hi, y1lo, y1hi;

            Ax = x1 - x0;
            Bx = x2 - x3;

            // X bound box test:
            if (Ax < 0) {
                x1lo = (short)x1;
                x1hi = (short)x0;
            } else {
                x1hi = (short)x1;
                x1lo = (short)x0;
            }
            if (Bx > 0) {
                if (x1hi < (short)x3 || (short)x2 < x1lo) {
                    continue;
                }
            } else {
                if (x1hi < (short)x2 || (short)x3 < x1lo) {
                    continue;
                }
            }

            Ay = y1 - y0;
            By = y2 - y3;

            // Y bound box test
            if (Ay < 0) {
                y1lo = (short)y1;
                y1hi = (short)y0;
            } else {
                y1hi = (short)y1;
                y1lo = (short)y0;
            }
            if (By > 0) {
                if (y1hi < (short)y3 || (short)y2 < y1lo) {
                    continue;
                }
            } else {
                if (y1hi < (short)y2 || (short)y3 < y1lo) {
                    continue;
                }
            }

            Cx = x0 - x2;
            Cy = y0 - y2;
            d = By * Cx - Bx * Cy; // alpha numerator
            f = Ay * Bx - Ax * By; // both denominator
            // alpha tests
            if (f > 0) {
                if (d < 0 || d > f) {
                    continue;
                }
            } else {
                if (d > 0 || d < f) {
                    continue;
                }
            }

            e = Ax * Cy - Ay * Cx; // beta numerator
            // beta tests
            if (f > 0) {
                if (e < 0 || e > f) {
                    continue;
                }
            } else {
                if (e > 0 || e < f) {
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
    return m_ShapeLines;
}

void Shape::setPosFromCentre()
{
    // Convert all lines to be relative to the centre of the shape
    auto centreX = m_MinX + (m_Width / 2);
    auto centreY = m_MinY + (m_Height / 2);
    setPos(centreX, centreY);
    for (auto& sl : m_ShapeLines) {
        sl.x0 -= centreX;
        sl.y0 -= centreY;
        sl.x1 -= centreX;
        sl.y1 -= centreY;
        sl.originalX0 -= centreX;
        sl.originalY0 -= centreY;
        sl.originalX1 -= centreX;
        sl.originalY1 -= centreY;
    }
}

void Shape::updateShapeSize(double x0, double y0, double x1, double y1)
{
    // update the size of the shape:
    if (x0 < m_MinX) {
        m_MinX = x0;
    }
    if (x0 > m_MaxX) {
        m_MaxX = x0;
    }
    if (x1 < m_MinX) {
        m_MinX = x1;
    }
    if (x1 > m_MaxX) {
        m_MaxX = x1;
    }
    if (y0 < m_MinY) {
        m_MinY = y0;
    }
    if (y0 > m_MaxY) {
        m_MaxY = y0;
    }
    if (y1 < m_MinY) {
        m_MinY = y1;
    }
    if (y1 > m_MaxY) {
        m_MaxY = y1;
    }

    m_Width = m_MaxX - m_MinX;
    m_Height = m_MaxY - m_MinY;
}

} // namespace amaze
} // namespace marengo
