#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

// Defines an adapter interface for the graphical toolkit
// in use (currently SfmlAdapter implements this)

namespace marengo {
namespace amaze {

struct MenuResults {
    int musicVolumePercent { 100 };
    int stickDeadZonePercent { 10 };
    std::string levelFileToLoad;
};

enum class KeyControls {
    LEFT,
    RIGHT,
    ACCELERATE,
    QUIT,
    LR_ANALOGUE, // for game controller
    PAUSE,
    MENU,
    START, // any other key or button; starts play when waiting to start
};

enum class MenuType {
    None, // normal game play
    Main, // Currently just one type of menu, we may want to add others
    OK, // menu has just exited with "OK"
    Cancel, // menu has just exited with "Cancel"
    Exit, // user selected "exit game"
};

struct Text {
    std::string text;
    uint8_t r { 0 };
    uint8_t g { 0 };
    uint8_t b { 0 };
    unsigned characterSize;
    std::optional<float> positionX; // no value means centered in X
    std::optional<float> positionY; // no value means centered in Y
};

class IGraphicsAdapter {
public:
    // TODO - make all functions const where possible
    virtual ~IGraphicsAdapter() { }
    virtual void setVsync(bool enabled) = 0;
    virtual void cls() = 0;
    virtual void redraw() = 0;
    virtual void
    drawLine(float xFrom, float yFrom, float xTo, float yTo, float width, int r, int g, int b) = 0;
    // Dimensions of the drawing area. This may be a virtual size rather than physical
    // pixels, e.g. if the adapter scales its drawing to the window.
    virtual int getWindowWidth() const = 0;
    virtual int getWindowHeight() const = 0;
    virtual int getTicks() const = 0;

    virtual void registerControlHandler(
        KeyControls key,
        std::function<void(const bool, const float)> controlHandler) = 0;
    virtual void processInput(bool paused) = 0;

    virtual void drawStatusBar() = 0;
    virtual void drawText(const Text& text) = 0;
    virtual MenuType menuDraw(MenuType menuType) = 0;
    virtual void menuProcessEvents() = 0;
    virtual MenuResults menuGetResults() = 0;
    virtual void setMouseCursorVisible(bool value) = 0;

    // Screem
    // Allows callers to scale things (currently just line drawing) according to the physical
    // dimensions of the screen
    virtual float getScalingFactor() = 0;

    // Sounds
    // load an sound & supply a key to reference it:
    virtual void soundLoad(const std::string& key, const std::string& filename) = 0;
    virtual void soundPlay(const std::string& key) = 0;
    virtual void soundLoop(const std::string& key, float volume) = 0;
    virtual void soundFade(const std::string& key, const int msecs) = 0;
    virtual void musicLoad(const std::string& filename) = 0;
    virtual void musicPlayLoop() = 0;
    virtual void setMusicVolumePercent(float vol) = 0;

    // Physical
    virtual void rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs)
        = 0;
};

} // namespace amaze
} // namespace marengo
