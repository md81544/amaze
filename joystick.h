#pragma once

// Note that we are using SDL, not SFML for gamepad input
// SFML's handling of gamepads is a bit erratic, whereas 
// SDL normalises all gamepads such that right stick is always reported
// rather SFML's method of presenting axes that may or may not exist depending
// the gamepad type. All SDL code is encapsulated within this class and init
// and quit is handled via RAII.

namespace marengo {
namespace amaze {
namespace joystick {

class Joystick {
public:
    Joystick();
    ~Joystick();

private:

};

} // namespace joystick
} // namespace amaze
} // namespace marengo
