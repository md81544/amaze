#include "gameshape.h"

namespace marengo
{
namespace amaze
{

GameShape::GameShape()
{
}

bool GameShape::IsActive() const
{
    return m_isActive;
}

void GameShape::SetIsActive(bool value)
{
    m_isActive = value;
}

GameShapeType GameShape::GetGameShapeType() const
{
    return m_gameShapeType;
}

void GameShape::SetGameShapeType( const GameShapeType& value )
{
    m_gameShapeType = value;
}


std::string GameShape::GetName() const
{
    return m_name;
}

void GameShape::SetName( const std::string& name )
{
    m_name = name;
}

} // namespace amaze
} // namespace marengo
