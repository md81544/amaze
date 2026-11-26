#pragma once

#include "shape.h"

#include <optional>
#include <utility>
#include <vector>

// A shape's type affects little beyond what happens when the ship collides
// with it (Controller::collisionChecks()) and whether it moves, so it's an
// enum rather than subclasses.

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
    SPECIAL // can move, spin and/or pull the ship with gravity
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

    // The following are only used for special objects
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
    float getPulsateAmount() const;
    void setPulsateAmount(float amount);
    // Following a path (see LEVEL_FORMAT.md) rather than using the deltas
    // above: it moves from its starting position through each offset in
    // turn, then back again, or if loop, straight back to the start and round
    // again. speed is the distance moved along the path per tick.
    void setPath(std::vector<std::pair<double, double>> offsets, double speed, bool loop);
    bool hasPath() const;
    const std::vector<std::pair<double, double>>& getPathOffsets() const;
    double getPathSpeed() const;
    bool getPathLoops() const;

private:
    bool m_isActive { true };
    std::string m_name; // for debugging only
    GameShapeType m_gameShapeType { GameShapeType::UNINITIALISED };

    // The following are only used for special objects
    std::optional<double> m_originalX { std::nullopt };
    std::optional<double> m_originalY { std::nullopt };
    float m_rotationDelta { 0.0f };
    float m_xDelta { 0.0f };
    float m_yDelta { 0.0f };
    float m_xMaxDifference { 0.0f };
    float m_yMaxDifference { 0.0f };
    float m_gravity { 0.f };
    float m_pulsateAmount { 0.001f };

    // The path, if any (see setPath())
    void moveAlongPath();
    // The offset from the starting position at a distance along the path
    std::pair<double, double> pathOffsetAt(double distance) const;
    std::vector<std::pair<double, double>> m_pathOffsets;
    double m_pathSpeed { 0.0 };
    bool m_pathLoops { false };
    // The path's points (starting with the start, (0, 0), and if it loops,
    // ending with it too), and the distance along the path of each
    std::vector<std::pair<double, double>> m_pathPoints;
    std::vector<double> m_pathDistances;
    // How far along the path it is, and which way it's going
    double m_pathDistance { 0.0 };
    bool m_pathForwards { true };
};

} // namespace amaze
} // namespace marengo
