#include "controller.h"
#include "gameshape.h"
#include "helperfunctions.h"
#include "log.h"

#include <chrono>
#include <memory>
#include <thread>

namespace marengo {
namespace amaze {

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
}

void Controller::registerControlHandlers()
{
    // We register callbacks with the graphics manager to avoid us
    // having to refer to (for example) SFML-specific keycodes outside of
    // the graphics manager. Using lambdas here but std::function can
    // contain functions, lambda expressions, bind expressions, or other
    // function objects.

    // Rotate Left
    m_graphicsAdapter.registerControlHandler(KeyControls::LEFT, [&](const bool isKeyDown, float) {
        if (isKeyDown) {
            m_gameModel.getShipModel()->setRotationDelta(1);
        } else {
            // if we are already rotating in this direction:
            if (helperfunctions::Sgn(m_gameModel.getShipModel()->rotationDelta()) == 1) {
                m_gameModel.getShipModel()->setRotationDelta(0);
            }
        }
    });

    // Rotate with analogue stick
    m_graphicsAdapter.registerControlHandler(
        KeyControls::LR_ANALOGUE, [&](const bool /* isKeyDown */, const float value) {
            double rotationDelta = value / 50.0;
            if (std::abs(rotationDelta) < 0.25) {
                rotationDelta = 0.0;
            }
            m_gameModel.getShipModel()->setRotationDelta(rotationDelta);
        });

    // Rotate Right
    m_graphicsAdapter.registerControlHandler(
        KeyControls::RIGHT, [&](const bool isKeyDown, const float) {
            if (isKeyDown) {
                m_gameModel.getShipModel()->setRotationDelta(-1);
            } else {
                // if we are already rotating in this direction:
                if (helperfunctions::Sgn(m_gameModel.getShipModel()->rotationDelta()) == -1) {
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

    // Quit
    m_graphicsAdapter.registerControlHandler(
        KeyControls::QUIT, [&](const bool isKeyDown, const float) {
            if (isKeyDown) {
                m_gameModel.setGameState(GameState::Quit);
            }
        });
}

void Controller::mainLoop(int gameLevel)
{
    // This is the main game control structure, called from main()

    // TODO splash screen?
    bool endingLevel = false;
    m_graphicsAdapter.setFrameRate(100);

    // Main game loop
    for (;;) {
        m_gameModel.getShipModel()->setVisible(!endingLevel);
        m_gameModel.levelLoad(gameLevel);
        m_gameModel.setGameState(GameState::Running);

        // Main loop per "life"
        for (;;) {
            m_scheduler.processSchedule();
            if (endingLevel) {
                goto end_loops;
            }
            m_gameModel.savePosition();

            switch (m_gameModel.getGameState()) {
                case GameState::Menu:
                    /*KeyControls key = */m_graphicsAdapter.processMenuInput();
                    break;
                case GameState::Paused:
                    m_graphicsAdapter.processInput(true);
                    m_view.stopSounds();
                    break;
                case GameState::Dead:
                    // We get here when the ship has finished exploding
                    if (m_gameModel.lifeLost() != 0) {
                        m_gameModel.restart();
                        break;
                    }
                    m_scheduler.doAfter(ScheduleEventName::EndLoops, 300, [&]() {
                        endingLevel = true; // all lives lost
                    });
                    break;
                case GameState::Quit:
                    // Break out of two loops, a justified use of goto :)
                    goto end_loops;
                    break;
                case GameState::Succeeded:
                    // TODO something more than just quit
                    m_view.stopSounds();
                    m_gameModel.getShipModel()->shipGameShape()->resize(1.2);
                    m_gameModel.getShipModel()->setIsAccelerating(false);
                    m_gameModel.getShipModel()->flamesGameShape()->setVisible(false);
                    m_gameModel.setBreakableExploding(false);
                    m_scheduler.doAfter(ScheduleEventName::EndLoops, 300, [&]() {
                        endingLevel = true;
                    });
                    break;
                case GameState::Exploding:
                    m_gameModel.process(); // perform all processing required per loop
                    break;
                case GameState::Running:
                    m_graphicsAdapter.processInput(false);
                    m_gameModel.process(); // perform all processing required per loop
                    collisionChecks();
                    break;
                default:
                    break;
            }
            m_graphicsAdapter.cls();
            m_view.update();
            m_graphicsAdapter.redraw();
        }
    }
end_loops:
}

void Controller::collisionChecks()
{
    // Collision detection
    auto collider = m_gameModel.collisionDetect();
    if (collider) {
        switch (collider->getGameShapeType()) {
            case GameShapeType::EXIT:
                m_graphicsAdapter.soundPlay("success");
                m_gameModel.setGameState(GameState::Succeeded);
                break;
            case GameShapeType::FUEL:
                m_graphicsAdapter.soundPlay("collect");
                collider->setIsActive(false);
                m_gameModel.extraLife();
                break;
            case GameShapeType::KEY:
                // currently an idea but not used
                break;
            case GameShapeType::NEUTRAL:
                break;
            case GameShapeType::OBSTRUCTION:
                m_gameModel.getShipModel()->setIsExploding(true);
                m_gameModel.setGameState(GameState::Exploding);
                break;
            case GameShapeType::BREAKABLE:
                if (m_gameModel.lifeLost() > 0) {
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
