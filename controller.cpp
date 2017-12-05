#include "controller.h"
#include "gameshape.h"
#include "helperfunctions.h"

#include "boost/filesystem.hpp"

#include <memory>
#include <chrono>

namespace marengo
{
namespace amaze
{

Controller::Controller(
    Preferences& p, GameModel& m, View& v, IGraphicsAdapter& g )
    : m_preferences( p ), m_gameModel( m ), m_view( v ), m_graphicsAdapter( g )
{
    registerControlHandlers();

    boost::filesystem::path dataPath( m_gameModel.getDataPath() );

    // Load sounds...
    m_graphicsAdapter.soundLoad(
        "rocket", ( dataPath / "rocket.wav" ).string() );
    m_graphicsAdapter.soundLoad(
        "collision", ( dataPath / "collision.wav" ).string() );
    m_graphicsAdapter.soundLoad(
        "collect", ( dataPath / "collect.wav" ).string() );
    m_graphicsAdapter.soundLoad(
        "success", ( dataPath / "success.wav" ).string() );
}

void Controller::splashScreen()
{
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
        KeyControls::LEFT, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.getShipModel()->setRotationDelta( 2 );
            }
            else
            {
                // if we are already rotating in this direction:
                if ( helperfunctions::Sgn(
                         m_gameModel.getShipModel()->rotationDelta() ) == 1 )
                {
                    m_gameModel.getShipModel()->setRotationDelta( 0 );
                }
            }
        } );

    // Rotate Right
    m_graphicsAdapter.registerControlHandler(
        KeyControls::RIGHT, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.getShipModel()->setRotationDelta( -2 );
            }
            else
            {
                // if we are already rotating in this direction:
                if ( helperfunctions::Sgn(
                         m_gameModel.getShipModel()->rotationDelta() ) == -1 )
                {
                    m_gameModel.getShipModel()->setRotationDelta( 0 );
                }
            }
        } );

    // Accelerate
    m_graphicsAdapter.registerControlHandler(
        KeyControls::ACCELERATE, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.getShipModel()->setIsAccelerating( true );
            }
            else
            {
                m_gameModel.getShipModel()->setIsAccelerating( false );
            }
        } );

    // Brake
    m_graphicsAdapter.registerControlHandler(
        KeyControls::BRAKE, [&]( const bool /* isKeyDown */ ) {
            // TODO
        } );

    // Quit
    m_graphicsAdapter.registerControlHandler(
        KeyControls::QUIT, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.setGameIsRunning( false );
            }
        } );
}

void Controller::run()
{
    // This is the main game control structure, called from main().
    size_t gameLevel = 0;

    splashScreen();

    m_gameModel.levelLoad( gameLevel );
    m_gameModel.setGameIsRunning( true );

    // Main game loop
    while ( true )
    {
        auto loopStart = std::chrono::system_clock::now();

        // Read any key presses and call any registered functions for those
        // keypresses
        m_graphicsAdapter.processInput();

        m_gameModel.process(); // perform all processing required per loop

        collisionChecks(); // check input etc and updates model
        if ( m_gameModel.gameIsRunning() == false )
        {
            break;
        }
        m_graphicsAdapter.cls();
        m_view.Update();
        m_graphicsAdapter.redraw();
        // m_GraphicsAdapter.loopDelay(loopStart, 20); // ensure loop lasts at
        // least n msecs
        auto loopEnd = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            loopEnd -
            loopStart ).count();
        m_gameModel.updateStatistics( millis );
    }
}

void Controller::collisionChecks()
{
    // Collision detection
    std::shared_ptr<GameShape> collider = m_gameModel.collisionDetect();
    if ( collider )
    {
        switch ( collider->GetGameShapeType() )
        {
        case GameShapeType::EXIT:
            m_graphicsAdapter.soundPlay( "success" );
            m_gameModel.setGameIsRunning( false ); // TODO next Level etc
            break;
        case GameShapeType::FUEL:
            m_graphicsAdapter.soundPlay( "collect" );
            collider->SetIsActive( false );
            // TODO - refuel
            break;
        case GameShapeType::KEY:
            // currently an idea but not used
            break;
        case GameShapeType::NEUTRAL:
            break;
        case GameShapeType::OBSTRUCTION:
            m_gameModel.getShipModel()->setIsExploding( true );
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
