#include "sfmladapter.h"
#include "helperfunctions.h"

namespace marengo
{
namespace amaze
{

SfmlAdapter::SfmlAdapter( int screenWidth, int screenHeight )
    : m_window( sf::VideoMode( screenWidth, screenHeight ), "Amaze" ),
      m_screenHeight( screenHeight ),
      m_screenWidth( screenWidth )
{
    // m_window.setMouseCursorVisible(false);
}

SfmlAdapter::~SfmlAdapter() {}

int SfmlAdapter::getScreenHeight() const { return m_screenHeight; }

int SfmlAdapter::getScreenWidth() const { return m_screenWidth; }

void SfmlAdapter::cls() { m_window.clear(); }

void SfmlAdapter::redraw() { m_window.display(); }

int SfmlAdapter::setDrawColour(
    uint8_t, // r
    uint8_t, // g
    uint8_t, // b
    uint8_t  // a
    )
{
    // TODO
    return 0;
}

void SfmlAdapter::drawLine(
    int, // xFrom,
    int, // xTo,
    int, // yTo,
    int  // width
    )
{
    // TODO call SFML
}

void SfmlAdapter::fillRectangle(
    int, // x,
    int, // y,
    int, // w,
    int  // h
    )
{
    // TODO
}

uint32_t SfmlAdapter::getTicks() const
{
    return 0; // TODO
}

void SfmlAdapter::delay( uint32_t /* milliseconds */ ) const
{
    // TODO
}

void SfmlAdapter::loopDelay(
    uint32_t, // previousTicks,
    uint32_t  // totalLoopMilliseconds
    ) const
{
    // TODO
}

size_t SfmlAdapter::imageLoad( const std::string& /* fileName */ )
{
    return 0; // TODO DELETE?
}

void SfmlAdapter::imageDisplay(
    size_t, // id,
    int,    // x,
    int     // y
    )
{
    // TODO - delete?
}

void SfmlAdapter::imageUnload( size_t /* id */ )
{
    // TODO - delete?
}

void SfmlAdapter::imageDisplay(
    const std::string&, // fileName,
    int,                // x,
    int                 // y
    )
{
    // TODO - delete?
}

void SfmlAdapter::registerControlHandler(
    KeyControls key, std::function<void( const bool )> controlHandler )
{
    m_ControlHandlers[ key ] = controlHandler;
}

void SfmlAdapter::processInput()
{
    return; // TODO
}

void SfmlAdapter::soundLoad(
    const std::string&, // key,
    const std::string&  // filename
    )
{
    // TODO
}

void SfmlAdapter::soundPlay( const std::string& /* key */ ) {}

void SfmlAdapter::soundLoop( const std::string& /* key */ ) {}

void SfmlAdapter::soundFade( const std::string& /* key */, const int /* msecs */)
{
    // TODO
}

} // namespace amaze
} // namespace marengo
