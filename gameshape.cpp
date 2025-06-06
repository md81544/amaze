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
    if (!m_originalX.has_value()) {
        m_originalX = getPosX();
    }
    if (!m_originalY.has_value()) {
        m_originalY = getPosY();
    }

    if (getPosX() > *m_originalX + m_xMaxDifference) {
        m_xDelta = -m_xDelta;
    }
    if (getPosX() < *m_originalX - m_xMaxDifference) {
        m_xDelta = -m_xDelta;
    }
    if (getPosY() > *m_originalY + m_yMaxDifference) {
        m_yDelta = -m_yDelta;
    }
    if (getPosY() < *m_originalY - m_yMaxDifference) {
        m_yDelta = -m_yDelta;
    }
    setPos(getPosX() + m_xDelta, getPosY() + m_yDelta);

    if (m_rotationDelta != 0.0f) {
        rotate(m_rotationDelta);
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

float GameShape::getGravity() const
{
    return m_gravity;
}

void GameShape::setGravity(float value)
{
    m_gravity = value;
}

float GameShape::getPulsateAmount() const
{
    return m_pulsateAmount;
}

void GameShape::setPulsateAmount(float amount)
{
    m_pulsateAmount = amount;
}

} // namespace amaze
} // namespace marengo
