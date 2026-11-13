#pragma once

#include "gamemodel.h"
#include "gameshell.h"
#include "scheduler.h"
#include "view.h"

#include <chrono>
#include <string>

// This class acts as the MVC "controller" - i.e. it contains
// game logic, tests input, collisions, end events etc

namespace marengo {
namespace amaze {

class Audio;
class Settings;

// The game simulation runs at a fixed rate, independent of the display's
// refresh rate. Each displayed frame runs as many simulation ticks as are
// needed to catch up with real time.

class Controller {
public:
    Controller(GameModel& m, View& v, GameShell& shell, Audio& audio, Settings& settings);
    // Loads the initial level; call once before the first frame()
    void start(int gameLevel, const std::string& levelFile);
    // Processes one displayed frame, drawing it with the painter. Returns
    // false when the game should exit.
    bool frame(QPainter& painter, const QSizeF& canvasSize);

private:
    void processInput(); // once per displayed frame
    void processMenu();
    void beginLevel(); // shows "Get Ready" until the player presses something
    void startPlayIfReady();
    void loadLevel(const std::string& filename); // also remembers it for next time
    void saveMenuOptions();
    void tick(); // once per simulation tick
    void render(QPainter& painter, const QSizeF& canvasSize); // once per displayed frame
    void collisionChecks();
    void bounceShip(); // off whatever obstacle it's touching
    void registerControlHandlers();

    GameModel& m_gameModel;
    View& m_view;
    GameShell& m_shell;
    Audio& m_audio;
    Settings& m_settings;
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
