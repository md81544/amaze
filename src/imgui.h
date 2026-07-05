#pragma once

// Class to completely contain all imgui-sfml code.
// Imgui-sfml is used to provide menus and other UI widgets.
// This is used directly from sfmladapter.cpp so no need to
// provide virtual interfaces etc.
//
// When the game state indicates a menu should be displayed,
// push all events through this class's processEvent() function.
//
// If the menu is visible, the main loop should call this class's build(MenuType) and
// render() function after doing other window draws.

#include "igraphicsadapter.h"
#include "levelfilelister.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

class Imgui {
public:
    explicit Imgui(sf::RenderWindow& window, const std::string& dataDir);
    ~Imgui() noexcept;
    void processEvents();
    void build(marengo::amaze::MenuType menuType);
    marengo::amaze::MenuType render();

    // Getters for widget states
    bool getQuitGame()
    {
        return m_quitGame;
    }

    int getBackgroundMusicVolumePercent()
    {
        return m_backgroundMusicVolume;
    }

    int getDeadZonePercent()
    {
        return m_deadZonePercent;
    }

private:
    sf::RenderWindow& m_window;
    int m_backgroundMusicVolume { 40 };
    bool m_quitGame { false };
    int m_deadZonePercent { 5 };
    int m_fileSelectedIndex { -1 };
    marengo::amaze::MenuType m_menuState{ marengo::amaze::MenuType::None };
    sf::Clock m_deltaClock;
    marengo::amaze::LevelFileLister m_levelFileLister;
};