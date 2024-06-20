#include "sfmladapter.h"
#include "exceptions.h"
#include "helperfunctions.h"

#include <chrono>
#include <thread>

namespace marengo
{
namespace amaze
{

SfmlAdapter::SfmlAdapter( int screenWidth, int screenHeight )
    : m_window( sf::VideoMode( screenWidth, screenHeight ), "Amaze" ),
      m_screenHeight( screenHeight ), m_screenWidth( screenWidth )
{
    m_window.setMouseCursorVisible( false );
}

SfmlAdapter::~SfmlAdapter() {}

int SfmlAdapter::getScreenHeight() const { return m_screenHeight; }

int SfmlAdapter::getTicks() const
{
    sf::Clock clock;
    return clock.getElapsedTime().asMilliseconds();
}

int SfmlAdapter::getScreenWidth() const { return m_screenWidth; }

void SfmlAdapter::cls() { m_window.clear(); }

void SfmlAdapter::redraw() { m_window.display(); }

int SfmlAdapter::setDrawColour( uint8_t, // r
    uint8_t,                             // g
    uint8_t,                             // b
    uint8_t                              // a
)
{
    // TODO
    return 0;
}

void SfmlAdapter::drawLine(
    int xFrom, int yFrom, int xTo, int yTo, int width, int r, int g, int b )
{
    // Note, apparently SFML uses a rectangle shape to draw a line with
    // thickness, so this may need revisiting in future (TODO?)
    sf::Vertex line[] = { sf::Vertex( sf::Vector2f( xFrom, yFrom ) ),
        sf::Vertex( sf::Vector2f( xTo, yTo ) ) };
    line[ 0 ].color = sf::Color( r, g, b );
    line[ 1 ].color = sf::Color( r, g, b );
    m_window.draw( line, 2, sf::Lines );
}

void SfmlAdapter::fillRectangle( int, // x,
    int,                              // y,
    int,                              // w,
    int                               // h
)
{
    // TODO
}

void SfmlAdapter::loopDelay( uint32_t previousTicks, // milliseconds
    uint32_t totalLoopMilliseconds ) const
{
    sf::Clock clock;
    while ( clock.getElapsedTime().asMilliseconds() <
            previousTicks + totalLoopMilliseconds )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }
}

size_t SfmlAdapter::imageLoad( const std::string& /* fileName */ )
{
    return 0; // TODO DELETE?
}

void SfmlAdapter::imageDisplay( size_t, // id,
    int,                                // x,
    int                                 // y
)
{
    // TODO - delete?
}

void SfmlAdapter::imageUnload( size_t /* id */ )
{
    // TODO - delete?
}

void SfmlAdapter::imageDisplay( const std::string&, // fileName,
    int,                                            // x,
    int                                             // y
)
{
    // TODO - delete?
}

void SfmlAdapter::registerControlHandler(
    KeyControls key, std::function<void( const bool )> controlHandler )
{
    m_controlHandlers[ key ] = controlHandler;
}

void SfmlAdapter::processInput()
{
    sf::Event event;

    while ( m_window.pollEvent( event ) )
    {
        switch ( event.type )
        {
        case sf::Event::KeyPressed:
            switch ( event.key.code )
            {
            case sf::Keyboard::Escape:
                m_controlHandlers[ KeyControls::QUIT ]( true );
                break;
            case sf::Keyboard::Up:
                m_controlHandlers[ KeyControls::ACCELERATE ]( true );
                break;
            case sf::Keyboard::Left:
                m_controlHandlers[ KeyControls::LEFT ]( true );
                break;
            case sf::Keyboard::Right:
                m_controlHandlers[ KeyControls::RIGHT ]( true );
                break;
            default:
                break;
            }
            break;
        case sf::Event::KeyReleased:
            switch ( event.key.code )
            {
            case sf::Keyboard::Up:
                m_controlHandlers[ KeyControls::ACCELERATE ]( false );
                break;
            case sf::Keyboard::Left:
                m_controlHandlers[ KeyControls::LEFT ]( false );
                break;
            case sf::Keyboard::Right:
                m_controlHandlers[ KeyControls::RIGHT ]( false );
                break;
            default:
                break;
            }
            break;

        // we don't process other types of events currently
        // TODO - game controller support
        default:
            break;
        }
    }
    return;
}

void SfmlAdapter::soundLoad(
    const std::string& key, const std::string& filename )
{
    auto buffer = std::make_shared<sf::SoundBuffer>();
    if ( !buffer->loadFromFile( filename ) )
    {
        THROWUP( AmazeRuntimeException, "Sound file load error" );
    }
    m_soundBuffers[ key ] = buffer;
    auto sound = std::make_shared<sf::Sound>();
    sound->setBuffer( *buffer );
    m_sounds[ key ] = sound;
}

void SfmlAdapter::soundPlay( const std::string& key )
{
    m_sounds[ key ]->setVolume( 100.f );
    if ( m_sounds[ key ]->getStatus() != sf::Sound::Playing )
    {
        m_sounds[ key ]->play();
    }
}

void SfmlAdapter::soundLoop( const std::string& key )
{
    m_sounds[ key ]->setVolume( 100.f );
    if ( m_sounds[ key ]->getStatus() != sf::Sound::Playing )
    {
        m_sounds[ key ]->setLoop( true );
        m_sounds[ key ]->play();
    }
}

void SfmlAdapter::soundFade( const std::string& key, const int /* msecs */ )
{
    // We don't have a fadeOut() type function in SFML so we reduce a bit on each call
    float newVolume = m_sounds[ key ]->getVolume() - 3.f;
    if ( newVolume < 0.f )
        newVolume = 0.f;
    m_sounds[ key ]->setVolume( newVolume );
}

} // namespace amaze
} // namespace marengo
