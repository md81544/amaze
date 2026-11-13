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

struct MenuResults {
    int musicVolumePercent { 100 };
    int stickDeadZonePercent { 10 };
    std::string levelFileToLoad;
};

} // namespace amaze
} // namespace marengo
