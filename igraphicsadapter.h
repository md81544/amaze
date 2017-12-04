#pragma once

#include <functional>

// Defines an adapter interface for the graphical toolkit
// in use (currently SfmlAdapter implements this)

namespace marengo
{
namespace amaze
{

enum class KeyControls
{
    LEFT,
    RIGHT,
    ACCELERATE,
    BRAKE,
    QUIT
};

class IGraphicsAdapter
{
public:
    // TODO - make all functions const where possible
    virtual ~IGraphicsAdapter() {}
    virtual void Cls() = 0;
    virtual void Redraw() = 0;
    virtual int SetDrawColour(
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a
        ) = 0;
    virtual void DrawLine( int xFrom, int xTo, int yTo, int width ) = 0;
    virtual void FillRectangle( int x, int y, int w, int h ) = 0;
    virtual int GetScreenWidth() const = 0;
    virtual int GetScreenHeight() const = 0;
    virtual uint32_t GetTicks() const = 0;
    virtual void Delay( uint32_t milliseconds ) const = 0;
    // delay incorporating loop processing time:
    virtual void LoopDelay(
        uint32_t previousTicks, uint32_t totalLoopMilliseconds ) const = 0;

    virtual void RegisterControlHandler(
        KeyControls key,
        std::function<void( const bool )> controlHandler
        ) = 0;
    virtual void ProcessInput() = 0;

    // Images
    // Load & display an image; any resources should be immediately discarded
    virtual void ImageDisplay( const std::string& fileName, int x, int y ) = 0;
    // load an image, some kind of reference to it and return an arbitrary ID
    // for it:
    virtual size_t imageLoad( const std::string& fileName ) = 0;
    // display a loaded image:
    virtual void ImageDisplay( size_t id, int x, int y ) = 0;
    // explicitly unload an image (otherwise it should be done when this object
    // destructs
    virtual void ImageUnload( size_t id ) = 0;

    // Sounds
    // load an sound & supply a key to reference it:
    virtual void SoundLoad(
        const std::string& key, const std::string& filename ) = 0;
    virtual void SoundPlay( const std::string& key ) = 0;
    virtual void SoundLoop( const std::string& key ) = 0;
    virtual void SoundFade( const std::string& key, const int msecs ) = 0;
};

} // namespace amaze
} // namespace marengo
