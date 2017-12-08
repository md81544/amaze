#pragma once

// All SFML-specific code should go in this class

#include "igraphicsadapter.h"

#include "SFML/Graphics.hpp"

#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

namespace marengo
{
namespace amaze
{

// TODO: add support for whole shapes via sf::LineStrip
// (add to IGraphicsAdapter)

class SfmlAdapter : public IGraphicsAdapter
{

public:
    SfmlAdapter( int screenWidth, int screenHeight );
    ~SfmlAdapter();
    virtual void cls() override;
    virtual void redraw() override;
    virtual int setDrawColour(
        uint8_t r, uint8_t g, uint8_t b, uint8_t a ) override;
    virtual void drawLine( int xFrom, int xTo, int yTo, int width ) override;
    virtual void fillRectangle( int x, int y, int w, int h ) override;
    virtual int getScreenWidth() const override;
    virtual int getScreenHeight() const override;
    virtual uint32_t getTicks() const override;
    virtual void delay( uint32_t milliseconds ) const override;
    virtual void loopDelay( uint32_t previousTicks,
        uint32_t totalLoopMilliseconds ) const override;
    virtual void imageDisplay(
        const std::string& fileName, int x, int y ) override;
    virtual size_t imageLoad( const std::string& fileName ) override;
    virtual void imageDisplay( size_t id, int x, int y ) override;
    virtual void imageUnload( size_t id ) override;
    virtual void registerControlHandler( KeyControls key,
        std::function<void( const bool )> controlHandler ) override;
    virtual void processInput() override;
    // Sound processing
    virtual void soundLoad(
        const std::string& key, const std::string& filename ) override;
    virtual void soundPlay( const std::string& key ) override;
    virtual void soundLoop( const std::string& key ) override;
    virtual void soundFade( const std::string& key, const int msecs ) override;

private:
    sf::RenderWindow m_window;
    int m_screenHeight;
    int m_screenWidth;

    std::unordered_map<KeyControls, std::function<void( const bool )>>
        m_ControlHandlers;
};

} // namespace amaze
} // namespace marengo
