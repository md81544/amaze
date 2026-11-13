#include "controller.h"
#include "gamemodel.h"
#include "gameshape.h"
#include "igraphicsadapter.h"
#include "log.h" // IWYU pragma: keep
#include "utils.h"

#include <algorithm>
#include <filesystem>
#include <memory>

namespace marengo {
namespace amaze {

namespace {

// Simulation rate. All per-tick values (physics, schedule durations, etc.) are
// tuned for this rate.
constexpr int TICKS_PER_SECOND = 120;
constexpr std::chrono::nanoseconds TICK_DURATION { 1'000'000'000 / TICKS_PER_SECOND };
// If a frame takes longer than this (e.g. window dragged, level loading),
// don't try to catch up all the lost time.
constexpr std::chrono::milliseconds MAX_FRAME_TIME { 250 };
constexpr int RESPAWN_DELAY_TICKS = 2 * TICKS_PER_SECOND;
// Log frames or input processing that take longer than this (diagnostic only)
constexpr std::chrono::milliseconds STALL_LOG_THRESHOLD { 50 };

} // namespace

Controller::Controller(GameModel& m, View& v, IGraphicsAdapter& g)
    : m_gameModel(m)
    , m_view(v)
    , m_graphicsAdapter(g)
{
    registerControlHandlers();

    std::filesystem::path dataPath(m_gameModel.getDataPath());

    // Load sounds...
    m_graphicsAdapter.soundLoad("rocket", (dataPath / "rocket.wav").string());
    m_graphicsAdapter.soundLoad("collision", (dataPath / "collision.wav").string());
    m_graphicsAdapter.soundLoad("collect", (dataPath / "collect.wav").string());
    m_graphicsAdapter.soundLoad("success", (dataPath / "success.wav").string());
    m_graphicsAdapter.soundLoad("breakable", (dataPath / "breakable.wav").string());
    m_graphicsAdapter.musicLoad((dataPath / "music.ogg").string());
}

void Controller::registerControlHandlers()
{
    // We register callbacks with the graphics manager to avoid us
    // having to refer to (for example) SFML-specific keycodes outside of
    // the graphics manager. Using lambdas here but std::function can
    // contain functions, lambda expressions, bind expressions, or other
    // function objects.

    // Rotate Left
    m_graphicsAdapter.registerControlHandler(
        KeyControls::LEFT, [&](const bool isKeyDown, const float value) {
            if (isKeyDown) {
                m_gameModel.getShipModel()->setRotationDelta(value);
            } else {
                // if we are already rotating in this direction:
                if (utils::sgn(m_gameModel.getShipModel()->rotationDelta()) == 1) {
                    m_gameModel.getShipModel()->setRotationDelta(0);
                }
            }
        });

    // Rotate with analogue stick
    m_graphicsAdapter.registerControlHandler(
        KeyControls::LR_ANALOGUE, [&](const bool /* isKeyDown */, const float value) {
            double rotationDelta = value;
            if (std::abs(rotationDelta) < 0.01) {
                rotationDelta = 0.0;
            }
            m_gameModel.getShipModel()->setRotationDelta(rotationDelta);
        });

    // Rotate Right
    m_graphicsAdapter.registerControlHandler(
        KeyControls::RIGHT, [&](const bool isKeyDown, const float value) {
            if (isKeyDown) {
                m_gameModel.getShipModel()->setRotationDelta(value);
            } else {
                // if we are already rotating in this direction:
                if (utils::sgn(m_gameModel.getShipModel()->rotationDelta()) == -1) {
                    m_gameModel.getShipModel()->setRotationDelta(0);
                }
            }
        });

    // Accelerate
    m_graphicsAdapter.registerControlHandler(
        KeyControls::ACCELERATE, [&](const bool isKeyDown, const float acceleration) {
            if (isKeyDown) {
                float accln = acceleration / 1000.f;
                m_gameModel.getShipModel()->setIsAccelerating(true, accln);
            } else {
                m_gameModel.getShipModel()->setIsAccelerating(false, 0.f);
            }
        });

    // Pause
    m_graphicsAdapter.registerControlHandler(
        KeyControls::PAUSE, [&](const bool isKeyDown, const float) {
            int now = m_graphicsAdapter.getTicks();
            if (isKeyDown
                && (m_gameModel.getGameState() == GameState::Running
                    || m_gameModel.getGameState() == GameState::Paused)
                && now > m_lastPause + 500) {
                m_gameModel.togglePause();
                m_lastPause = m_graphicsAdapter.getTicks();
            }
        });

    // Menu
    m_graphicsAdapter.registerControlHandler(KeyControls::MENU, [&](const bool, const float) {
        m_gameModel.setGameState(GameState::Menu);
    });

    // Quit
    m_graphicsAdapter.registerControlHandler(
        KeyControls::QUIT, [&](const bool isKeyDown, const float) {
            if (isKeyDown) {
                m_gameModel.setGameState(GameState::Quit);
            }
        });
}

void Controller::mainLoop(int gameLevel, const std::string& levelFile)
{
    // This is the main game control structure, called from main()
    start(gameLevel, levelFile);
    while (frame()) { }
}

void Controller::start(int gameLevel, const std::string& levelFile)
{
    // TODO splash screen?
    m_graphicsAdapter.setVsync(true);
    m_graphicsAdapter.musicPlayLoop();

    if (!levelFile.empty()) {
        m_gameModel.levelLoad(levelFile);
    } else {
        m_gameModel.levelLoad(gameLevel);
    }
    m_gameModel.setGameState(GameState::Running);

    m_lastFrameTime = std::chrono::steady_clock::now();
    m_accumulator = {};
}

bool Controller::frame()
{
    using namespace std::chrono;

    auto beforeInput = steady_clock::now();
    auto elapsed = beforeInput - m_lastFrameTime;
    if (elapsed > STALL_LOG_THRESHOLD) {
        mgo::Log::debug(
            std::format("Frame interval of {}ms", duration_cast<milliseconds>(elapsed).count()));
    }
    m_accumulator += std::min<steady_clock::duration>(elapsed, MAX_FRAME_TIME);

    processInput();

    // Time spent processing input is excluded from simulation time. Event polling
    // can block (e.g. on macOS the first key press can take a few hundred ms while
    // text input services initialise); counting that time would make the
    // simulation catch up in one burst with the new input already applied,
    // causing e.g. a sudden jump in rotation.
    auto afterInput = steady_clock::now();
    if (afterInput - beforeInput > STALL_LOG_THRESHOLD) {
        mgo::Log::debug(
            std::format(
                "Input processing took {}ms",
                duration_cast<milliseconds>(afterInput - beforeInput).count()));
    }
    m_lastFrameTime = afterInput;

    while (m_accumulator >= TICK_DURATION) {
        if (m_gameModel.getGameState() == GameState::Quit) {
            return false;
        }
        tick();
        m_accumulator -= TICK_DURATION;
    }
    if (m_gameModel.getGameState() == GameState::Quit) {
        return false;
    }

    render();
    return true;
}

void Controller::processInput()
{
    switch (m_gameModel.getGameState()) {
        case GameState::Menu:
            processMenu();
            break;
        case GameState::Paused:
            m_graphicsAdapter.processInput(true);
            break;
        case GameState::Running:
            m_graphicsAdapter.setMouseCursorVisible(false);
            m_graphicsAdapter.processInput(false);
            break;
        default:
            break;
    }
}

void Controller::processMenu()
{
    m_graphicsAdapter.setMouseCursorVisible(true);
    if (m_currentMenu == MenuType::None) {
        m_currentMenu = MenuType::Main;
        m_graphicsAdapter.rumble(0, 0, 0); // turn off any rumbles
        m_gameModel.getShipModel()->setIsAccelerating(false, 0.f);
        m_view.stopSounds();
    }
    m_graphicsAdapter.menuProcessEvents(); // process SFML events
    // Adjust background music immediately
    m_graphicsAdapter.setMusicVolumePercent(m_graphicsAdapter.menuGetResults().musicVolumePercent);
    if (m_currentMenu == MenuType::OK || m_currentMenu == MenuType::Cancel) {
        // User closed dialog with OK or Cancel
        if (m_currentMenu == MenuType::OK) {
            MenuResults results = m_graphicsAdapter.menuGetResults();
            if (!results.levelFileToLoad.empty()) {
                m_gameModel.levelLoad(results.levelFileToLoad);
            }
            // TODO: joytick deadzone
        }
        m_currentMenu = MenuType::None;
        m_gameModel.getShipModel()->setVisible(true);
        m_gameModel.setGameState(GameState::Running);
    }
    if (m_currentMenu == MenuType::Exit) {
        m_gameModel.setGameState(GameState::Quit);
    }
}

void Controller::tick()
{
    m_scheduler.processSchedule();
    m_gameModel.savePosition();
    switch (m_gameModel.getGameState()) {
        case GameState::Paused:
            m_graphicsAdapter.rumble(0, 0, 0); // turn off any rumbles
            m_view.stopSounds();
            break;
        case GameState::Dead:
            // We get here when the ship has finished exploding
            m_graphicsAdapter.rumble(0, 0, 0);
            if (m_scheduler.isScheduleItemActive(ScheduleEventName::Respawn)
                || m_scheduler.isScheduleItemActive(ScheduleEventName::LevelEnded)) {
                // Already decided what happens next
                break;
            }
            if (m_gameModel.lifeLost() != 0) {
                // Leave the explosion on screen briefly before respawning
                m_scheduler.doAfter(ScheduleEventName::Respawn, RESPAWN_DELAY_TICKS, [&]() {
                    m_gameModel.restart();
                });
                break;
            }
            m_scheduler.doAfter(ScheduleEventName::LevelEnded, 300, [&]() {
                // We just reload the level, but display the menu
                // in case the user wants a different level
                m_gameModel.levelLoad(m_gameModel.levelFileName());
                m_gameModel.getShipModel()->setVisible(false);
                m_gameModel.setGameState(GameState::Menu);
            });
            break;
        case GameState::Succeeded:
            m_view.stopSounds();
            m_graphicsAdapter.rumble(0, 0, 0); // turn off any rumbles
            m_gameModel.getShipModel()->shipGameShape()->resize(1.2);
            m_gameModel.getShipModel()->setIsAccelerating(false);
            m_gameModel.getShipModel()->flamesGameShape()->setVisible(false);
            m_gameModel.setBreakableExploding(false);
            m_scheduler.doAfter(ScheduleEventName::LevelEnded, 300, [&]() {
                // We just reload the level, but display the menu
                // in case the user wants a different level
                m_gameModel.levelLoad(m_gameModel.levelFileName());
                m_gameModel.getShipModel()->setVisible(false);
                m_gameModel.setGameState(GameState::Menu);
            });
            break;
        case GameState::Exploding:
            m_graphicsAdapter.rumble(0xFFFF, 0xFFFF, 1000);
            m_gameModel.process(); // perform all processing required per tick
            break;
        case GameState::Running:
            m_gameModel.process(); // perform all processing required per tick
            collisionChecks();
            break;
        default:
            break;
    }
    m_view.playSounds();
}

void Controller::render()
{
    m_graphicsAdapter.cls();
    m_view.draw();
    if (m_gameModel.getGameState() == GameState::Menu) {
        m_currentMenu = m_graphicsAdapter.menuDraw(m_currentMenu);
    }
    m_graphicsAdapter.redraw();
}

void Controller::collisionChecks()
{
    // Collision detection
    auto colliderTuple = m_gameModel.collisionDetect();
    // collider is a tuple of GameShapeType and shared_ptr<GameShape>
    std::shared_ptr<GameShape> collider = std::get<std::shared_ptr<GameShape>>(colliderTuple);
    GameShapeType collideeType = std::get<GameShapeType>(colliderTuple);
    if (collider) {
        switch (collider->getGameShapeType()) {
            case GameShapeType::EXIT:
                if (collideeType != GameShapeType::FLAMES) {
                    m_graphicsAdapter.soundPlay("success");
                    m_gameModel.setGameState(GameState::Succeeded);
                }
                break;
            case GameShapeType::FUEL:
                if (collideeType != GameShapeType::FLAMES) {
                    m_graphicsAdapter.soundPlay("collect");
                    collider->setIsActive(false);
                    m_gameModel.extraLife();
                }
                break;
            case GameShapeType::KEY:
                // currently an idea but not used
                break;
            case GameShapeType::NEUTRAL:
                break;
            case GameShapeType::OBSTRUCTION:
            case GameShapeType::MOVING:
                if (collideeType != GameShapeType::FLAMES) {
                    m_gameModel.getShipModel()->setIsExploding(true);
                    m_gameModel.setGameState(GameState::Exploding);
                }
                break;
            case GameShapeType::BREAKABLE:
                if (collideeType == GameShapeType::FLAMES) {
                    m_graphicsAdapter.soundPlay("breakable");
                    m_gameModel.setBreakableExploding();
                } else if (m_gameModel.lifeLost() > 0) {
                    m_graphicsAdapter.soundPlay("breakable");
                    m_gameModel.setBreakableExploding();
                } else {
                    m_gameModel.getShipModel()->setIsExploding(true);
                    m_gameModel.setGameState(GameState::Exploding);
                }
                collider->setIsActive(false); // Breakable objects can be destroyed
                break;
            case GameShapeType::PRISONER:
                // currently an idea but not used
                break;
            case GameShapeType::UNINITIALISED:
                break;
            default:
                break;
        }
    }
}

} // namespace amaze
} // namespace marengo
