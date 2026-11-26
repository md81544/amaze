#pragma once

// Gamepad input uses SDL (Qt 6 has no gamepad support). SDL normalises all
// gamepads, e.g. the right stick is always reported the same way whatever
// the gamepad type. All SDL code is encapsulated within this class and init
// and quit is handled via RAII.

// Note all axes are reported in the range -1.f to 1.f

#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace marengo {
namespace amaze {
namespace gamepad {

enum class ButtonType {
    South, // A or cross
    East, // B or circle
    West, // X or square
    North, // Y or triangle
    Start,
    Back,
    LeftShoulder,
    RightShoulder,
    DPadDown,
    DPadLeft,
    DPadRight,
    DPadUp,
    Unknown,
    NotApplicable,
};

enum class ButtonAction {
    Pressed,
    Released,
    NotApplicable,
};

enum class EventType {
    ButtonPressed,
    ButtonReleased,
    Analogue,
    Connect,
    Disconnect,
    Unknown,
};

struct AnalogueStatus {
    float leftX { 0.f };
    float leftY { 0.f };
    float rightX { 0.f };
    float rightY { 0.f };
    float leftTrigger { 0.f };
    float rightTrigger { 0.f };
};

struct Event {
    uint32_t joystickId { 0 };
    EventType eventType { EventType::Unknown };
    ButtonType buttonType { ButtonType::NotApplicable };
    ButtonAction buttonAction { ButtonAction::NotApplicable };
    AnalogueStatus analogue;
};

class Gamepad {
public:
    Gamepad();
    ~Gamepad();
    // This should be called once per game frame. It returns a vector
    // of events that have happened since the last call.
    std::vector<Event> getEvents();
    bool isGamePadAttached();
    // Intensities are scaled by the rumble intensity (see setRumbleIntensity)
    void rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs);
    // Scales all rumbles, from 0 (off) to 1 (full strength, the default)
    void setRumbleIntensity(float fraction);
    // Stick movement within this fraction (0 to 1) of centre is reported as zero
    void setDeadZone(float fraction);

private:
    std::unordered_map<SDL_JoystickID, SDL_Gamepad*> m_gamepads;
    // If several gamepads are attached, the one last used (button pressed or
    // stick/trigger moved) is reported and rumbled
    SDL_JoystickID m_activeGamepad { 0 };
    // Raw (unprocessed) axis values for each gamepad
    std::unordered_map<SDL_JoystickID, AnalogueStatus> m_analogueStatus;
    AnalogueStatus m_previousAnalogueStatus;
    float m_deadZone { 0.05f };
    float m_rumbleIntensity { 1.f };
};

} // namespace gamepad
} // namespace amaze
} // namespace marengo
