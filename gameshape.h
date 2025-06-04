#pragma once

#include "shape.h"

#include <optional>

// TODO (perhaps?) might want to ditch the "gameShapeType"
// and having sub classes depending on type. As currently
// functionality is no different between different shape
// types this seems overkill.

namespace marengo {
namespace amaze {

enum class GameShapeType {
    UNINITIALISED,
    NEUTRAL,
    SHIP,
    FLAMES,
    OBSTRUCTION,
    FUEL,
    PRISONER,
    KEY,
    EXIT,
    BREAKABLE, // acts like OBSTRUCTION, but is destroyed once hit
    MOVING
};

class GameShape final : public Shape {
public:
    GameShape();

    bool IsActive() const;
    void setIsActive(bool value);

    GameShapeType getGameShapeType() const;
    void setGameShapeType(const GameShapeType& value);

    std::string getName() const;
    void setName(const std::string& name);

    // The following are only used for moving objects
    void move();
    float getRotationDelta() const;
    void setRotationDelta(float value);
    float getXDelta() const;
    float getYDelta() const;
    void setXDelta(float value);
    void setYDelta(float value);
    void setXMaxDifference(float value);
    void setYMaxDifference(float value);
    float getXMaxDifference() const;
    float getYMaxDifference() const;
    float getGravity() const;
    void setGravity(float value);

private:
    bool m_isActive { true };
    std::string m_name; // for debugging only
    GameShapeType m_gameShapeType { GameShapeType::UNINITIALISED };

    // The following are only used for moving objects
    std::optional<double> m_originalX { std::nullopt };
    std::optional<double> m_originalY { std::nullopt };
    float m_rotationDelta { 0.0f };
    float m_xDelta { 0.0f };
    float m_yDelta { 0.0f };
    float m_xMaxDifference { 0.0f };
    float m_yMaxDifference { 0.0f };
    //float m_gravity { 0.f };
};

} // namespace amaze
} // namespace marengo
