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
    RegisterControlHandlers();

    boost::filesystem::path dataPath( m_gameModel.GetDataPath() );

    // Load sounds...
    m_graphicsAdapter.SoundLoad(
        "rocket", ( dataPath / "rocket.wav" ).string() );
    m_graphicsAdapter.SoundLoad(
        "collision", ( dataPath / "collision.wav" ).string() );
    m_graphicsAdapter.SoundLoad(
        "collect", ( dataPath / "collect.wav" ).string() );
    m_graphicsAdapter.SoundLoad(
        "success", ( dataPath / "success.wav" ).string() );
}

void Controller::SplashScreen() {}

void Controller::RegisterControlHandlers()
{
    // We register callbacks with the graphics manager to avoid us
    // having to refer to (for example) SFML-specific keycodes outside of
    // the graphics manager. Using lambdas here but std::function can
    // contain functions, lambda expressions, bind expressions, or other
    // function objects.

    // Rotate Left
    m_graphicsAdapter.RegisterControlHandler(
        KeyControls::LEFT, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.ShipModel()->setRotationDelta( 2 );
            }
            else
            {
                // if we are already rotating in this direction:
                if ( HelperFunctions::Sgn(
                         m_gameModel.ShipModel()->rotationDelta() ) == 1 )
                {
                    m_gameModel.ShipModel()->setRotationDelta( 0 );
                }
            }
        } );

    // Rotate Right
    m_graphicsAdapter.RegisterControlHandler(
        KeyControls::RIGHT, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.ShipModel()->setRotationDelta( -2 );
            }
            else
            {
                // if we are already rotating in this direction:
                if ( HelperFunctions::Sgn(
                         m_gameModel.ShipModel()->rotationDelta() ) == -1 )
                {
                    m_gameModel.ShipModel()->setRotationDelta( 0 );
                }
            }
        } );

    // Accelerate
    m_graphicsAdapter.RegisterControlHandler(
        KeyControls::ACCELERATE, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.ShipModel()->setIsAccelerating( true );
            }
            else
            {
                m_gameModel.ShipModel()->setIsAccelerating( false );
            }
        } );

    // Brake
    m_graphicsAdapter.RegisterControlHandler(
        KeyControls::BRAKE, [&]( const bool isKeyDown ) {
            // TODO
        } );

    // Quit
    m_graphicsAdapter.RegisterControlHandler(
        KeyControls::QUIT, [&]( const bool isKeyDown ) {
            if ( isKeyDown )
            {
                m_gameModel.setGameIsRunning( false );
            }
        } );
}

void Controller::Run()
{
    // This is the main game control structure, called from main().
    size_t gameLevel = 0;

    SplashScreen();

    m_gameModel.LevelLoad( gameLevel );
    m_gameModel.setGameIsRunning( true );

    // Main game loop
    while ( true )
    {
        auto loopStart = std::chrono::system_clock::now();

        // Read any key presses and call any registered functions for those
        // keypresses
        m_graphicsAdapter.ProcessInput();

        m_gameModel.Process(); // perform all processing required per loop

        CollisionChecks(); // check input etc and updates model
        if ( m_gameModel.GameIsRunning() == false )
            break;
        m_graphicsAdapter.Cls();
        m_view.Update();
        m_graphicsAdapter.Redraw();
        // m_GraphicsAdapter.loopDelay(loopStart, 20); // ensure loop lasts at
        // least n msecs
        auto loopEnd = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            loopEnd -
            loopStart ).count();
        m_gameModel.UpdateStatistics( millis );
    }
}

void Controller::CollisionChecks()
{
    // Collision detection
    std::shared_ptr<GameShape> collider = m_gameModel.CollisionDetect();
    if ( collider )
    {
        switch ( collider->GetGameShapeType() )
        {
        case GameShapeType::EXIT:
            m_graphicsAdapter.SoundPlay( "success" );
            m_gameModel.setGameIsRunning( false ); // TODO next Level etc
            break;
        case GameShapeType::FUEL:
            m_graphicsAdapter.SoundPlay( "collect" );
            collider->SetIsActive( false );
            // TODO - refuel
            break;
        case GameShapeType::KEY:
            // currently an idea but not used
            break;
        case GameShapeType::NEUTRAL:
            break;
        case GameShapeType::OBSTRUCTION:
            m_gameModel.ShipModel()->setIsExploding( true );
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
