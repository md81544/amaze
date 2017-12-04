#pragma once

// All SFML-specific code should go in this class

#include "igraphicsadapter.h"

#include "SFML/Graphics.hpp"
#include "SFML/Audio.hpp"

#include <unordered_map>
#include <map>
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
    virtual void Cls() override;
    virtual void Redraw() override;
    virtual int SetDrawColour(
        uint8_t r, uint8_t g, uint8_t b, uint8_t a ) override;
    virtual void DrawLine( int xFrom, int xTo, int yTo, int width ) override;
    virtual void FillRectangle( int x, int y, int w, int h ) override;
    virtual int GetScreenWidth() const override;
    virtual int GetScreenHeight() const override;
    virtual uint32_t GetTicks() const override;
    virtual void Delay( uint32_t milliseconds ) const override;
    virtual void LoopDelay( uint32_t previousTicks,
        uint32_t totalLoopMilliseconds ) const override;
    virtual void ImageDisplay(
        const std::string& fileName, int x, int y ) override;
    virtual size_t imageLoad( const std::string& fileName ) override;
    virtual void ImageDisplay( size_t id, int x, int y ) override;
    virtual void ImageUnload( size_t id ) override;
    virtual void RegisterControlHandler( KeyControls key,
        std::function<void( const bool )> controlHandler ) override;
    virtual void ProcessInput() override;
    // Sound processing
    virtual void SoundLoad(
        const std::string& key, const std::string& filename ) override;
    virtual void SoundPlay( const std::string& key ) override;
    virtual void SoundLoop( const std::string& key ) override;
    virtual void SoundFade( const std::string& key, const int msecs ) override;

private:
    sf::RenderWindow m_Window;
    int m_ScreenWidth;
    int m_ScreenHeight;

    std::map<KeyControls, std::function<void( const bool )>> m_ControlHandlers;
};

} // namespace amaze
} // namespace marengo
