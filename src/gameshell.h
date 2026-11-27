#pragma once

// The game's window and input: owns the main window (GameWidget) and the
// menu dialog, turns keyboard and gamepad input into KeyControls for the
// registered handlers, and handles full screen and the mouse cursor.
// A QApplication must exist before this is constructed.
//
// Qt owns the event loop, so run() is called with a function to be called
// once per displayed frame.
//
// Drawing is done on a virtual canvas which always has the same area as a
// 1920x1200 screen, with its shape following the window's. Everything drawn
// therefore scales uniformly as the window is resized.

#include "controls.h"
#include "gamepad.h"

#include <QElapsedTimer>
#include <QRect>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

class QPainter;
class QSizeF;

namespace marengo {
namespace amaze {

class GameWidget;
class LevelProgress;
class MenuDialog;
struct GameInputEvent;

class GameShell final {
public:
    using FrameFunction = std::function<bool(QPainter& painter, const QSizeF& canvasSize)>;
    using ControlHandler = std::function<void(const bool isKeyDown, const float value)>;

    // windowGeometry, if supplied (and on screen), takes precedence over
    // windowWidth/Height. initialMenuValues are the options the menu starts
    // with; the stick dead zone is applied immediately.
    GameShell(
        unsigned windowWidth,
        unsigned windowHeight,
        std::optional<QRect> windowGeometry,
        bool useFullScreen,
        const MenuResults& initialMenuValues,
        const std::string& dataDir);
    ~GameShell();

    // Shows the window and runs the Qt event loop, calling frameFunction once
    // per displayed frame until it returns false. The painter is already
    // scaled to the canvas, whose size is supplied. Exceptions thrown by
    // frameFunction are rethrown from here.
    void run(FrameFunction frameFunction);

    // Milliseconds since startup
    int ticks() const;

    void registerControlHandler(KeyControls key, ControlHandler controlHandler);
    // Calls the control handlers for input received since the last call.
    // Called once per frame, unless the menu is active.
    void processInput(bool paused);

    // Menu: call updateMenu() once per frame while the menu should be shown
    // (it opens the menu if necessary); it returns OK/Cancel (once) or Exit
    // when the menu is closed. processMenuInput() is called once per frame
    // instead of processInput() while the menu is active.
    MenuType updateMenu(MenuType menuType);
    void processMenuInput();
    MenuResults menuResults() const;
    // Updates which levels the menu lets the player choose, selecting the
    // current one. Call before the menu opens.
    void updateMenuLevels(const LevelProgress& progress, const std::string& currentLevelFile);

    void setMouseCursorVisible(bool value);
    void setStickDeadZonePercent(int percent);
    // Scales all gamepad rumble; 0 turns it off
    void setRumbleIntensityPercent(int percent);
    // The window's normal (i.e. not full screen) geometry, and whether it's
    // full screen, e.g. for saving between runs. Geometry may be invalid if
    // the window has only been shown full screen.
    QRect windowGeometry() const;
    bool isFullScreen() const;
    void rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs);

private:
    // Ratio of window (logical) pixels to virtual canvas pixels
    double canvasScale() const;
    void processGameKeyEvent(const GameInputEvent& event, bool paused);
    // Calls the rotate / thrust handlers for the keys held, at the strength
    // for the current shift state
    void applyHeldRotation();
    void applyHeldThrust();
    // Releases any rotate / thrust keys held, e.g. when their releases will go
    // elsewhere because the window has lost focus
    void releaseHeldKeys();
    void processGamepadForGame();
    // Starts, adjusts or stops the rumble for right-stick thrust (0 to 1)
    void updateThrustRumble(float thrust);
    void processGamepadForMenu();
    void sendMenuKey(int key, bool pressed);
    void toggleFullScreen();
    void updateCursor();

    std::unique_ptr<GameWidget> m_window;
    MenuDialog* m_menu; // owned by m_window
    bool m_useFullScreen;
    QElapsedTimer m_clock;

    std::unordered_map<KeyControls, ControlHandler> m_controlHandlers;
    gamepad::Gamepad m_gamepad;
    bool m_gamepadInUse { false };
    // Whether the current rumble is the thrust one (see updateThrustRumble)
    bool m_thrustRumbling { false };
    // Keyboard rotate / thrust keys currently held: the most recently pressed
    // direction (-1, 0 or 1; for rotation, 1 is left; for thrust, 1 is up), and
    // whether shift (fine control) is held
    int m_heldRotation { 0 };
    int m_heldThrust { 0 };
    bool m_shiftHeld { false };
    bool m_cursorVisible { false };
    // In full screen (while we're the active application) the cursor is also
    // hidden at system level, as that takes effect immediately; Qt cursor
    // changes only take effect when the mouse next moves
    bool m_systemCursorHidden { false };

    MenuType m_menuState { MenuType::None };
    bool m_menuOpenPending { false };
    // Stick-driven menu navigation: the arrow key currently held via a stick,
    // and when to next auto-repeat it
    int m_menuStickKey { 0 };
    qint64 m_menuStickRepeatAt { 0 };
};

} // namespace amaze
} // namespace marengo
