#include "controller.h"
#include "gameshape.h"
#include "helperfunctions.h"
#include "log.h"

#include "boost/filesystem.hpp"

#include <chrono>
#include <memory>

namespace marengo {
namespace amaze {

Controller::Controller(Preferences& p, GameModel& m, View& v, IGraphicsAdapter& g)
    : m_preferences(p)
    , m_gameModel(m)
    , m_view(v)
    , m_graphicsAdapter(g)
{
    registerControlHandlers();

    boost::filesystem::path dataPath(m_gameModel.getDataPath());

    // Load sounds...
    m_graphicsAdapter.soundLoad("rocket", (dataPath / "rocket.wav").string());
    m_graphicsAdapter.soundLoad("collision", (dataPath / "collision.wav").string());
    m_graphicsAdapter.soundLoad("collect", (dataPath / "collect.wav").string());
    m_graphicsAdapter.soundLoad("success", (dataPath / "success.wav").string());
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
        KeyControls::LR_ANALOGUE, [&](const bool isKeyDown, const float value) {
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

    // Quit
    m_graphicsAdapter.registerControlHandler(
        KeyControls::QUIT, [&](const bool isKeyDown, const float) {
            if (isKeyDown) {
                m_gameModel.setGameIsRunning(false);
            }
        });
}

void Controller::mainLoop(int gameLevel)
{
    // This is the main game control structure, called from main().

    // TODO splash screen?

    m_gameModel.levelLoad(gameLevel);
    m_gameModel.setGameIsRunning(true);

    // Main game loop
    while (true) {
        int loopStart = m_graphicsAdapter.getTicks();

        // Read any key presses and call any registered functions for those
        // keypresses
        m_graphicsAdapter.processInput();

        m_gameModel.process(); // perform all processing required per loop

        // collisionChecks();

        if (m_gameModel.gameIsRunning() == false) {
            break;
        }
        m_graphicsAdapter.cls();
        m_view.Update();
        m_graphicsAdapter.redraw();
        m_graphicsAdapter.loopDelay(loopStart, 10); // ensure loop lasts at least n msecs
    }
}

void Controller::collisionChecks()
{
    // Collision detection
    std::shared_ptr<GameShape> collider = m_gameModel.collisionDetect();
    if (collider) {
        switch (collider->GetGameShapeType()) {
        case GameShapeType::EXIT:
            m_graphicsAdapter.soundPlay("success");
            m_gameModel.setGameIsRunning(false); // TODO next Level etc
            break;
        case GameShapeType::FUEL:
            m_graphicsAdapter.soundPlay("collect");
            collider->SetIsActive(false);
            // TODO - refuel
            break;
        case GameShapeType::KEY:
            // currently an idea but not used
            break;
        case GameShapeType::NEUTRAL:
            break;
        case GameShapeType::OBSTRUCTION:
            m_gameModel.getShipModel()->setIsExploding(true);
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
