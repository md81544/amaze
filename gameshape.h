#pragma once

#include "shape.h"

// TODO (perhaps?) might want to ditch the "gameShapeType"
// and having sub classes depending on type. As currently
// functionality is no different between different shape
// types this seems overkill.

namespace marengo {
namespace amaze {

enum class GameShapeType {
    UNINITIALISED,
    NEUTRAL,
    OBSTRUCTION,
    FUEL,
    PRISONER,
    KEY,
    EXIT,
    BREAKABLE // acts like OBSTRUCTION, but is destroyed once hit
};

class GameShape : public Shape {
public:
    GameShape();

    bool IsActive() const;
    void setIsActive(bool value);

    GameShapeType getGameShapeType() const;
    void setGameShapeType(const GameShapeType& value);

    std::string getName() const;
    void setName(const std::string& name);

private:
    bool m_isActive { true };
    std::string m_name; // for debugging only
    GameShapeType m_gameShapeType { GameShapeType::UNINITIALISED };
};

} // namespace amaze
} // namespace marengo
