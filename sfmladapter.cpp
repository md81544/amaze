#include "sfmladapter.h"
#include "helperfunctions.h"

namespace marengo
{
namespace amaze
{

SfmlAdapter::SfmlAdapter( int screenWidth, int screenHeight )
    : m_Window( sf::VideoMode( screenWidth, screenHeight ), "Amaze" ),
      m_ScreenHeight( screenHeight ), m_ScreenWidth( screenWidth )
{
    // m_Window.setMouseCursorVisible(false);
}

SfmlAdapter::~SfmlAdapter() {}

int SfmlAdapter::GetScreenHeight() const { return m_ScreenHeight; }

int SfmlAdapter::GetScreenWidth() const { return m_ScreenWidth; }

void SfmlAdapter::Cls() { m_Window.clear(); }

void SfmlAdapter::Redraw() { m_Window.display(); }

int SfmlAdapter::SetDrawColour( uint8_t r, uint8_t g, uint8_t b, uint8_t a )
{
    // TODO
    return 0;
}

void SfmlAdapter::DrawLine( int xFrom, int xTo, int yTo, int width )
{
    // TODO call SFML
}

void SfmlAdapter::FillRectangle( int x, int y, int w, int h )
{
    // TODO
}

uint32_t SfmlAdapter::GetTicks() const
{
    return 0; // TODO
}

void SfmlAdapter::Delay( uint32_t milliseconds ) const
{
    // TODO
}

void SfmlAdapter::LoopDelay(
    uint32_t previousTicks, uint32_t totalLoopMilliseconds ) const
{
    // TODO
}

size_t SfmlAdapter::imageLoad( const std::string& fileName )
{
    return 0; // TODO DELETE?
}

void SfmlAdapter::ImageDisplay( size_t id, int x, int y )
{
    // TODO - delete?
}

void SfmlAdapter::ImageUnload( size_t id )
{
    // TODO - delete?
}

void SfmlAdapter::ImageDisplay( const std::string& fileName, int x, int y )
{
    // TODO - delete?
}

void SfmlAdapter::RegisterControlHandler(
    KeyControls key, std::function<void( const bool )> controlHandler )
{
    m_ControlHandlers[ key ] = controlHandler;
}

void SfmlAdapter::ProcessInput()
{
    return; // TODO
}

void SfmlAdapter::SoundLoad(
    const std::string& key, const std::string& filename )
{
    // TODO
}

void SfmlAdapter::SoundPlay( const std::string& key ) {}

void SfmlAdapter::SoundLoop( const std::string& key ) {}

void SfmlAdapter::SoundFade( const std::string& key, const int msecs )
{
    // TODO
}

} // namespace amaze
} // namespace marengo
