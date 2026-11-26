#pragma once

// Types shared between the GameShell (which turns keyboard / gamepad input
// and the menu into these) and the Controller (which acts on them)

#include <string>

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
    START, // any other key or button; starts play when waiting to start
};

enum class MenuType {
    None, // normal game play
    Main, // Currently just one type of menu, we may want to add others
    OK, // menu has just exited with "OK"
    Cancel, // menu has just exited with "Cancel"
    Exit, // user selected "exit game"
};

// Range offered for the stick dead zone. Zero isn't allowed: sticks rarely
// rest exactly at centre, and any input outside the dead zone turns the ship
// at a noticeable minimum speed, so too small a dead zone shows as drift.
constexpr int MIN_STICK_DEAD_ZONE_PERCENT = 3;
constexpr int MAX_STICK_DEAD_ZONE_PERCENT = 25;

struct MenuResults {
    int musicVolumePercent { 100 };
    int stickDeadZonePercent { 5 };
    std::string levelFileToLoad;
};

} // namespace amaze
} // namespace marengo
