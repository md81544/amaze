#pragma once

// All Qt-specific code should go in this class (and the GameWidget and
// MenuDialog classes it uses). A QApplication must exist before this is
// constructed.
//
// Unlike SFML, Qt owns the event loop, so instead of the controller running
// its own loop, run() is called with a function to be called once per
// displayed frame.
//
// Drawing is done on a virtual canvas which always has the same area as a
// 1920x1200 screen, with its shape following the window's. Everything drawn
// therefore scales uniformly as the window is resized.

#include "gamepad.h"
#include "igraphicsadapter.h"

#include <QAudioOutput>
#include <QElapsedTimer>
#include <QFont>
#include <QMediaPlayer>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class QPainter;
class QSoundEffect;

namespace marengo {
namespace amaze {

class GameWidget;
class MenuDialog;
struct GameInputEvent;

class QtAdapter final : public IGraphicsAdapter {
public:
    QtAdapter(
        unsigned windowWidth,
        unsigned windowHeight,
        bool useFullScreen,
        const std::string& dataDir);
    ~QtAdapter();

    // Shows the window and runs the Qt event loop, calling frameFunction once
    // per displayed frame until it returns false. Exceptions thrown by
    // frameFunction are rethrown from here.
    void run(std::function<bool()> frameFunction);

    virtual void setVsync(bool enabled) override;
    virtual void cls() override;
    virtual void redraw() override;
    virtual void
    drawLine(float xFrom, float yFrom, float xTo, float yTo, float width, int r, int g, int b)
        override;
    virtual int getWindowWidth() const override;
    virtual int getWindowHeight() const override;
    virtual int getTicks() const override;
    virtual void drawStatusBar() override;
    virtual void drawText(const Text& text) override;
    virtual MenuType menuDraw(MenuType menuType) override;
    virtual void menuProcessEvents() override;
    virtual MenuResults menuGetResults() override;
    virtual void setMouseCursorVisible(bool value) override;
    virtual void registerControlHandler(
        KeyControls key,
        std::function<void(const bool, const float)> controlHandler) override;
    virtual void processInput(bool paused) override;
    virtual void soundLoad(const std::string& key, const std::string& filename) override;
    virtual void soundPlay(const std::string& key) override;
    virtual void soundLoop(const std::string& key, float volume) override;
    virtual void soundFade(const std::string& key, const int msecs) override;
    virtual void musicLoad(const std::string& filename) override;
    virtual void musicPlayLoop() override;
    virtual void setMusicVolumePercent(float vol) override;
    virtual void
    rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs) override;
    virtual float getScalingFactor() override;

private:
    // Ratio of window (logical) pixels to virtual canvas pixels
    double canvasScale() const;
    void processGameKeyEvent(const GameInputEvent& event, bool paused);
    void processGamepadForGame();
    void processGamepadForMenu();
    void sendMenuKey(int key, bool pressed);
    QSoundEffect& sound(const std::string& key);

    std::unique_ptr<GameWidget> m_window;
    MenuDialog* m_menu; // owned by m_window
    QPainter* m_painter { nullptr }; // only valid during a frame
    bool m_useFullScreen;
    QElapsedTimer m_clock;
    QFont m_font;

    std::unordered_map<KeyControls, std::function<void(const bool, const float)>> m_controlHandlers;
    std::unordered_map<std::string, std::unique_ptr<QSoundEffect>> m_sounds;
    QAudioOutput m_musicOutput;
    QMediaPlayer m_music;
    gamepad::Gamepad m_gamepad;
    bool m_gamepadInUse { false };

    MenuType m_menuState { MenuType::None };
    bool m_menuOpenPending { false };
    // Stick-driven menu navigation: the arrow key currently held via a stick,
    // and when to next auto-repeat it
    int m_menuStickKey { 0 };
    qint64 m_menuStickRepeatAt { 0 };
};

} // namespace amaze
} // namespace marengo
