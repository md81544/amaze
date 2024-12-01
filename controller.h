#pragma once

#include "gamemodel.h"
#include "igraphicsadapter.h"
#include "scheduler.h"
#include "view.h"

#include <utility>

// This class acts as the MVC "controller" - i.e. it contains
// game logic, tests input, collisions, end events etc

namespace marengo {
namespace amaze {

class Controller {
public:
    Controller(GameModel& m, View& v, IGraphicsAdapter& s);
    void mainLoop(int gameLevel);
    void collisionChecks();
    void registerControlHandlers();

private:
    GameModel& m_gameModel;
    View& m_view;
    IGraphicsAdapter& m_graphicsAdapter;
    int m_lastPause { 0 };
    Scheduler m_scheduler;
};

} // namespace amaze
} // namespace marengo
