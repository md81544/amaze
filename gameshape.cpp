#include "gameshape.h"

namespace marengo {
namespace amaze {

GameShape::GameShape() { }

bool GameShape::IsActive() const
{
    return m_isActive;
}

void GameShape::setIsActive(bool value)
{
    m_isActive = value;
}

GameShapeType GameShape::getGameShapeType() const
{
    return m_gameShapeType;
}

void GameShape::setGameShapeType(const GameShapeType& value)
{
    m_gameShapeType = value;
}

std::string GameShape::getName() const
{
    return m_name;
}

void GameShape::setName(const std::string& name)
{
    m_name = name;
}

void GameShape::move()
{
    if (m_ShapeLines.empty()) {
        return;
    }
    auto& sl = m_ShapeLines[0];
    if (sl.x0 > sl.originalX0 + m_xMaxDifference) {
        m_xDelta = -m_xDelta;
    }
    if (sl.x0 < sl.originalX0 - m_xMaxDifference) {
        m_xDelta = -m_xDelta;
    }
    if (sl.y0 > sl.originalY0 + m_yMaxDifference) {
        m_yDelta = -m_yDelta;
    }
    if (sl.y0 < sl.originalY0 - m_yMaxDifference) {
        m_yDelta = -m_yDelta;
    }
    for (auto& sl : m_ShapeLines) {
        sl.x0 += m_xDelta;
        sl.y0 += m_yDelta;
        sl.x1 += m_xDelta;
        sl.y1 += m_yDelta;
    }
}

float GameShape::getRotationDelta() const
{
    return m_rotationDelta;
}

void GameShape::setRotationDelta(float value)
{
    m_rotationDelta = value;
}

float GameShape::getXDelta() const
{
    return m_xDelta;
}

float GameShape::getYDelta() const
{
    return m_yDelta;
}

void GameShape::setXDelta(float value)
{
    m_xDelta = value;
}

void GameShape::setYDelta(float value)
{
    m_yDelta = value;
}

void GameShape::setXMaxDifference(float value)
{
    m_xMaxDifference = value;
}

void GameShape::setYMaxDifference(float value)
{
    m_yMaxDifference = value;
}

float GameShape::getXMaxDifference() const
{
    return m_xMaxDifference;
}

float GameShape::getYMaxDifference() const
{
    return m_yMaxDifference;
}

} // namespace amaze
} // namespace marengo
