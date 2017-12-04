#pragma once

#include "shape.h"

// TODO (perhaps?) might want to ditching the "gameShapeType"
// and having sub classes depending on type. As currently
// functionality is no different between different shape
// types this seems overkill.

namespace marengo
{
namespace amaze
{

enum class GameShapeType
{
    UNINITIALISED,
    NEUTRAL,
    OBSTRUCTION,
    FUEL,
    PRISONER,
    KEY,
    EXIT
};

class GameShape : public Shape
{
public:
    GameShape();

    bool          IsActive() const;
    void          SetIsActive(bool value);

    GameShapeType GetGameShapeType() const;
    void          SetGameShapeType(const GameShapeType &value);

    std::string   GetName() const;
    void          SetName(const std::string &name);

private:
    bool          m_isActive { true };
    std::string   m_name; // for debugging only
    GameShapeType m_gameShapeType { GameShapeType::UNINITIALISED };
};

} // namespace amaze
} // namespace marengo
