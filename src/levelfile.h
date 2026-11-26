#pragma once

// Reads level (.lvl) files, in the JSON format described in LEVEL_FORMAT.md
// (read by levelformat.cpp), into the game's shapes.

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
