#pragma once

// All SFML-specific code should go in this class

#include "igraphicsadapter.h"
#include "menu.h"

#include "SFML/Audio.hpp"
#include "SFML/Graphics.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace marengo {
namespace amaze {

class SfmlAdapter final : public IGraphicsAdapter {

public:
    SfmlAdapter(
        unsigned screenWidth,
        unsigned screenHeight,
        bool useFullScreen,
        const std::string& dataDir);
    ~SfmlAdapter();
    virtual void setFrameRate(unsigned int fr) override;
    virtual void cls() override;
    virtual void redraw() override;
    virtual int setDrawColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
    virtual void
    drawLine(int xFrom, int yFrom, int xTo, int yTo, int width, int r, int g, int b) override;
    static int getPhysicalScreenWidth();
    static int getPhysicalScreenHeight();
    virtual int getWindoWidth() const override;
    virtual int getWindowHeight() const override;
    virtual int getTicks() const override;
    virtual void imageDisplay(const std::string& fileName, int x, int y) override;
    virtual size_t imageLoad(const std::string& fileName) override;
    virtual void imageDisplay(size_t id, int x, int y) override;
    virtual void imageUnload(size_t id) override;
    virtual void drawStatusBar() override;
    virtual void drawText(const Text& text) override;
    virtual void drawMenu(std::vector<MenuItem> menuItems, int currentlyHighlightedItem) override;
    virtual void registerControlHandler(
        KeyControls key,
        std::function<void(const bool, const float)> controlHandler) override;
    virtual void processInput(bool paused) override;
    virtual KeyControls processMenuInput() override;
    // Sound processing
    virtual void soundLoad(const std::string& key, const std::string& filename) override;
    virtual void soundPlay(const std::string& key) override;
    virtual void soundLoop(const std::string& key, float volume) override;
    virtual void soundFade(const std::string& key, const int msecs) override;

private:
    sf::RenderWindow m_window;
    int m_screenHeight;
    int m_screenWidth;

    std::unordered_map<KeyControls, std::function<void(const bool, const float)>> m_controlHandlers;
    std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> m_soundBuffers;
    std::unordered_map<std::string, std::shared_ptr<sf::Sound>> m_sounds;
    sf::Clock m_clock;
    sf::Font m_font; // we only use one font for all text currently
};

} // namespace amaze
} // namespace marengo
