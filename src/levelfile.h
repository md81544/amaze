#pragma once

// Reads level (.lvl) files. The file format is as follows:
// The first line should be "!~<timelimit>~<fuel>~<x>~<y>~<angle>~<description>"
// where x, y and angle give the ship's starting position, and timelimit and
// fuel are unused. The description is optional.
// Then each world object for the level is described as follows:
// Objects start with the line "N~<ObjectClass (text)>~<Unique ID>"
// (MOVING objects have further parameters; see readLevelFile())
// then the following can be used:
// L~<x0>~<y0>~<x1>~<y1>~<R>~<G>~<B>~<Thickness (optional) - draws a line
// P~<x>~<y> - set object's position
// T~<text>~<R>~<G>~<B>~<Thickness (optional) - create object from text
// All lines apply to the current object being described until a new "N~..."
// line is processed.

#include "gameshape.h"

#include <memory>
#include <string>
#include <vector>

namespace marengo {
namespace amaze {

struct LevelData {
    double shipX { 0.0 };
    double shipY { 0.0 };
    double shipRotation { 0.0 };
    std::string description;
    std::vector<std::unique_ptr<GameShape>> shapes;
};

// Throws AmazeRuntimeException if the file can't be read or is malformed
LevelData readLevelFile(const std::string& filename);

} // namespace amaze
} // namespace marengo
