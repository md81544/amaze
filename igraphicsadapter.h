#pragma once

#include "menu.h"

#include <cstdint>
#include <functional>
#include <optional>

// Defines an adapter interface for the graphical toolkit
// in use (currently SfmlAdapter implements this)

namespace marengo {
namespace amaze {

enum class KeyControls {
    LEFT,
    RIGHT,
    ACCELERATE,
    QUIT,
    LR_ANALOGUE, // for game controller
    PAUSE,
    MENU,
    UP, // only used for menus
    DOWN, // only used for menus
    ENTER, // only used for menus
    EXIT, // only used for menus
    NONE // only used for menus
};

struct Text {
    std::string text;
    uint8_t r { 0 };
    uint8_t g { 0 };
    uint8_t b { 0 };
    unsigned characterSize;
    std::optional<float> positionX;  // no value means centered in X
    std::optional<float> positionY;  // no value means centered in Y
};

class IGraphicsAdapter {
public:
    // TODO - make all functions const where possible
    virtual ~IGraphicsAdapter() { }
    virtual void setFrameRate(unsigned int fr) = 0;
    virtual void cls() = 0;
    virtual void redraw() = 0;
    virtual int setDrawColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
    virtual void drawLine(int xFrom, int yFrom, int xTo, int yTo, int width, int r, int g, int b)
        = 0;
    virtual int getWindoWidth() const = 0;
    virtual int getWindowHeight() const = 0;
    virtual int getTicks() const = 0;

    virtual void registerControlHandler(
        KeyControls key,
        std::function<void(const bool, const float)> controlHandler)
        = 0;
    virtual void processInput(bool paused) = 0;
    virtual KeyControls processMenuInput() = 0;

    // Images
    // Load & display an image; any resources should be immediately discarded
    virtual void imageDisplay(const std::string& fileName, int x, int y) = 0;
    // load an image, some kind of reference to it and return an arbitrary ID
    // for it:
    virtual size_t imageLoad(const std::string& fileName) = 0;
    // display a loaded image:
    virtual void imageDisplay(size_t id, int x, int y) = 0;
    // explicitly unload an image (otherwise it should be done when this object
    // destructs
    virtual void imageUnload(size_t id) = 0;
    virtual void drawStatusBar() = 0;
    virtual void drawText(const Text& text) = 0;
    virtual void drawMenu(std::vector<MenuItem> menuItems, int currentlyHighlightedItem) = 0;

    // Sounds
    // load an sound & supply a key to reference it:
    virtual void soundLoad(const std::string& key, const std::string& filename) = 0;
    virtual void soundPlay(const std::string& key) = 0;
    virtual void soundLoop(const std::string& key, float volume) = 0;
    virtual void soundFade(const std::string& key, const int msecs) = 0;
};

} // namespace amaze
} // namespace marengo
