#pragma once

#include "gameshape.h"

#include <functional>
#include <memory>
#include <vector>

// This is the interface for a model. Any model may be passed to
// the view's Update() function for display. This allows us to use
// the view to show the game, or splash screens, or menus, etc.

// Note the addressable area is arbitrarily 2000x2000 "units"
// in dimension. The viewport is 320x240 of these units.
// The default viewport position is (0,0).
// The View model displays the viewport at the size of
// the physical window and scales the "units" to pixels
// accordingly. SFML has "viewport" capability but a lot of this
// code was written for other graphics toolkits and was ported.
// Hence we don't use a lot of useful SFML stuff, and what we do
// use is abstracted.

namespace marengo {
namespace amaze {

class IModel {
public:
    // Get what the current "world" rotation is for this model
    virtual unsigned int getRotation() const = 0;
};

} // namespace amaze
} // namespace marengo
