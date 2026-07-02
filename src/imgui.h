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

private:
    sf::RenderWindow& m_window;
    // TODO real member variables, need to add getters as well
    // so we can query them from the caller
    bool m_enableThing = false;
    bool m_enableOtherThing = true;
    char m_nameBuffer[128] = "This is some text";
    bool m_quitGame { false };
    float m_deadZone = 0.05;
    marengo::amaze::MenuType m_menuState;
    sf::Clock m_deltaClock;
};