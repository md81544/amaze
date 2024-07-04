#pragma once

#include <cstdint>
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
    void setScale(double scale);
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

protected:
    std::vector<ShapeLine> m_ShapeLines;
    double m_Width { 0 };
    double m_Height { 0 };
    double m_X { 0 };
    double m_Y { 0 };
    double m_Scale { 1 };
    int m_Rotation { 0 };
    int m_PreviousRotation { 0 };
    bool m_IsVisible { true };
    double m_MinX { 0 };
    double m_MinY { 0 };
    double m_MaxX { 0 };
    double m_MaxY { 0 };
    // default colours for addLine:
    uint8_t m_R { 255 };
    uint8_t m_G { 255 };
    uint8_t m_B { 255 };
    uint8_t m_A { 255 };
    void updateShapeSize(double x0, double y0, double x1, double y1);
};

} // namespace amaze
} // namespace marengo
