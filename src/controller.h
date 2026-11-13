#pragma once

#include "gamemodel.h"
#include "igraphicsadapter.h"
#include "scheduler.h"
#include "view.h"

#include <chrono>
#include <string>

// This class acts as the MVC "controller" - i.e. it contains
// game logic, tests input, collisions, end events etc

namespace marengo {
namespace amaze {

// The game simulation runs at a fixed rate, independent of the display's
// refresh rate. Each displayed frame runs as many simulation ticks as are
// needed to catch up with real time.

class Controller {
public:
    Controller(GameModel& m, View& v, IGraphicsAdapter& s);
    // Calls start() then frame() until the game exits
    void mainLoop(int gameLevel, const std::string& levelFile);
    // Loads the initial level; call once before the first frame()
    void start(int gameLevel, const std::string& levelFile);
    // Processes one displayed frame. Returns false when the game should exit.
    bool frame();

private:
    void processInput(); // once per displayed frame
    void processMenu();
    void beginLevel(); // shows "Get Ready" until the player presses something
    void startPlayIfReady();
    void tick(); // once per simulation tick
    void render(); // once per displayed frame
    void collisionChecks();
    void registerControlHandlers();

    GameModel& m_gameModel;
    View& m_view;
    IGraphicsAdapter& m_graphicsAdapter;
    int m_lastPause { 0 };
    Scheduler m_scheduler;
    MenuType m_currentMenu { MenuType::None };
    // A level has been loaded but play hasn't started on it yet
    bool m_levelStartPending { false };
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::duration m_accumulator {};
};

} // namespace amaze
} // namespace marengo
