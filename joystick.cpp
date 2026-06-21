#include "joystick.h"
#include "exceptions.h"
#include <SDL3/SDL.h>

namespace marengo {
namespace amaze {
namespace joystick {

Joystick::Joystick()
{
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        THROWUP(AmazeRuntimeException, "Could not initialise SDL for joystick");
    }
}

Joystick::~Joystick()
{
    SDL_Quit();
}

} // namespace joystick
} // namespace amaze
} // namespace marengo