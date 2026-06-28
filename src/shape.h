#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace marengo {
namespace amaze {

struct ShapeLine {
    double x0;
    double y0;
    double x1;
    double y1;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    int lineThickness;
};

class Shape {
public:
    Shape();
    virtual ~Shape();
    void addShapeLine(ShapeLine sl);
    void setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void addLine(double x0, double y0, double x1, double y1, uint8_t thickness = 1);
    double getWidth() const;
    double getHeight() const;
    void setPos(double x, double y);
    double getPosX() const;
    double getPosY() const;
    bool isVisible() const;
    void setVisible(bool b);
    double getScale() const;
    void rotate(double rotationDelta);
    void resize(double scale);
    void makeFromText(
        const std::string& s,
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a,
        int lineThickness);
    void clear();
    bool intersectCheck(std::shared_ptr<Shape>) const;
    const std::vector<ShapeLine>& getVec() const;
    void setPosFromCentre();

protected:
    std::vector<ShapeLine> m_shapeLines;
    double m_width { 0.0 };
    double m_height { 0.0 };
    double m_x { 0.0 };
    double m_y { 0.0 };
    double m_scale { 1.0 };
    int m_rotation { 0 };
    int m_previousRotation { 0 };
    bool m_isVisible { true };
    double m_minX { std::numeric_limits<double>::max() };
    double m_minY { std::numeric_limits<double>::max() };
    double m_maxX { std::numeric_limits<double>::min() };
    double m_maxY { std::numeric_limits<double>::min() };
    // default colours for addLine:
    uint8_t m_r { 255 };
    uint8_t m_g { 255 };
    uint8_t m_b { 255 };
    uint8_t m_a { 255 };
    void updateShapeSize(double x0, double y0, double x1, double y1);
};

} // namespace amaze
} // namespace marengo
