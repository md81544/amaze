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

} // namespace amaze
} // namespace marengo
