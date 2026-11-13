#include "levelfile.h"
#include "exceptions.h"
#include "utils.h"

#include <format>
#include <fstream>

namespace marengo {
namespace amaze {

namespace {

GameShapeType shapeTypeFromName(const std::string& name)
{
    if (name == "OBSTRUCTION") {
        return GameShapeType::OBSTRUCTION;
    } else if (name == "FUEL") {
        return GameShapeType::FUEL;
    } else if (name == "PRISONER") {
        return GameShapeType::PRISONER;
    } else if (name == "KEY") {
        return GameShapeType::KEY;
    } else if (name == "EXIT") {
        return GameShapeType::EXIT;
    } else if (name == "BREAKABLE") {
        return GameShapeType::BREAKABLE;
    } else if (name == "MOVING") {
        return GameShapeType::MOVING;
    }
    return GameShapeType::NEUTRAL;
}

// Adds the object to the level. Lines before the first "N~..." line are
// discarded.
void addObject(LevelData& level, std::unique_ptr<GameShape>& obj)
{
    if (obj->getGameShapeType() != GameShapeType::UNINITIALISED) {
        if (obj->getGameShapeType() == GameShapeType::MOVING) {
            obj->setPosFromCentre(); // converts all lines to relative to the object's centre
        }
        level.shapes.push_back(std::move(obj));
    }
}

void parseLine(
    const std::vector<std::string>& vec,
    LevelData& level,
    std::unique_ptr<GameShape>& obj)
{
    switch (vec[0][0]) {
        case '!': // timelimit (unused), fuel (unused), ship x, ship y, ship angle, description
            if (vec.size() < 6) {
                throw AmazeRuntimeException("Malformed header line");
            }
            level.shipX = std::stod(vec[3]);
            level.shipY = std::stod(vec[4]);
            level.shipRotation = std::stod(vec[5]);
            if (vec.size() > 6) {
                level.description = vec[6];
            }
            break;
        case 'N': // New object, parameter 1 is type, parameter 2 is a name
            addObject(level, obj);
            obj = std::make_unique<GameShape>();
            obj->setGameShapeType(
                vec.size() > 1 ? shapeTypeFromName(vec[1]) : GameShapeType::NEUTRAL);
            // N~MOVING~<name>~<dx>~<max x diff>~<dy>~<max y diff>~<rotation delta>~<gravity
            // (optional)>
            if (obj->getGameShapeType() == GameShapeType::MOVING && vec.size() >= 8) {
                obj->setName(vec[2]);
                obj->setXDelta(std::stof(vec[3]));
                obj->setXMaxDifference(std::stof(vec[4]));
                obj->setYDelta(std::stof(vec[5]));
                obj->setYMaxDifference(std::stof(vec[6]));
                obj->setRotationDelta(std::stof(vec[7]));
                if (vec.size() >= 9) {
                    obj->setGravity(std::stof(vec[8]));
                }
            }
            break;
        case 'L':
            if (vec.size() == 8 || vec.size() == 9) {
                ShapeLine sl1 {
                    std::stod(vec[1]), // x0
                    std::stod(vec[2]), // y0
                    std::stod(vec[3]), // x1
                    std::stod(vec[4]), // y1
                    static_cast<uint8_t>(std::stoi(vec[5])), // r
                    static_cast<uint8_t>(std::stoi(vec[6])), // g
                    static_cast<uint8_t>(std::stoi(vec[7])), // b
                    255, // alpha
                    1 // thickness (default if not supplied)
                };
                if (vec.size() == 9) {
                    sl1.lineThickness = std::stoi(vec[8]);
                }
                obj->addShapeLine(sl1);
            }
            break;
        case 'P':
            if (vec.size() == 3) {
                obj->setPos(std::stoi(vec[1]), std::stoi(vec[2]));
            }
            break;
        case 'T':
            if (vec.size() == 5) {
                obj->makeFromText(
                    vec[1], std::stoi(vec[2]), std::stoi(vec[3]), std::stoi(vec[4]), 255, 6);
            }
            if (vec.size() == 6) {
                obj->makeFromText(
                    vec[1],
                    std::stoi(vec[2]),
                    std::stoi(vec[3]),
                    std::stoi(vec[4]),
                    255,
                    std::stoi(vec[5]));
            }
            break;
        default:
            break;
    }
}

} // namespace

LevelData readLevelFile(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in) {
        throw AmazeRuntimeException("Failed to load Level file " + filename);
    }

    LevelData level;
    bool haveHeader = false;
    auto obj = std::make_unique<GameShape>();
    std::string currentLine;
    int lineNumber = 0;
    while (std::getline(in, currentLine)) {
        ++lineNumber;
        std::vector<std::string> vec;
        utils::csvSplit(currentLine, '~', vec);
        if (vec.empty() || vec[0].empty()) {
            continue;
        }
        haveHeader = haveHeader || vec[0][0] == '!';
        try {
            parseLine(vec, level, obj);
        } catch (const std::exception& e) {
            // Includes stoi() etc. failing on a non-numeric value
            throw AmazeRuntimeException(
                std::format("Error in Level file {} line {}: {}", filename, lineNumber, e.what()));
        }
    }
    if (!haveHeader) {
        throw AmazeRuntimeException("Missing header line in Level file " + filename);
    }
    addObject(level, obj);
    return level;
}

} // namespace amaze
} // namespace marengo
