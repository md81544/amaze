#pragma once

#include "gamemodel.h"
#include "view.h"
#include "preferences.h"
#include "igraphicsadapter.h"

// This class acts as the MVC "controller" - i.e. it contains
// game logic, tests input, collisions, end events etc

namespace marengo
{
namespace amaze
{

class Controller
{
public:
    Controller(
        Preferences& p,
        GameModel& m,
        View& v,
        IGraphicsAdapter& s
        );
    void run();
    void collisionChecks();
    void registerControlHandlers();
    void splashScreen();

private:
    Preferences&      m_preferences;
    GameModel&        m_gameModel;
    View&             m_view;
    IGraphicsAdapter& m_graphicsAdapter;
};

} // namespace amaze
} // namespace marengo
