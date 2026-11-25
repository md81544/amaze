#include "levelfile.h"
#include "exceptions.h"
#include "levelformat.h"
#include "utils.h"

#include <cctype>
#include <cmath>
#include <format>
#include <fstream>
#include <sstream>

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

// ---------------------------------------------------------------------------
// The JSON format (see LEVEL_FORMAT.md), read by levelformat.cpp. Each item
// becomes a GameShape, just as the corresponding object in the old format
// does, with its appearance (colours and default thicknesses) as given in
// LEVEL_FORMAT.md.
// ---------------------------------------------------------------------------

struct Appearance {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    double thickness; // default
};
constexpr Appearance WALL { 255, 0, 0, mgo::levelformat::DEFAULT_WALL_THICKNESS };
constexpr Appearance BREAKABLE { 255, 150, 50, mgo::levelformat::DEFAULT_BREAKABLE_THICKNESS };
constexpr Appearance MOVING { 255, 172, 163, mgo::levelformat::DEFAULT_MOVING_OBJECT_THICKNESS };
constexpr Appearance EXIT { 52, 213, 235, mgo::levelformat::DEFAULT_EXIT_THICKNESS };
constexpr Appearance FUEL { 255, 255, 0, mgo::levelformat::DEFAULT_FUEL_THICKNESS };

std::unique_ptr<GameShape> newShape(GameShapeType type, const std::string& name)
{
    auto shape = std::make_unique<GameShape>();
    shape->setGameShapeType(type);
    shape->setName(name);
    return shape;
}

void addLines(
    GameShape& shape,
    const std::vector<mgo::levelformat::Polyline>& polylines,
    const Appearance& appearance)
{
    for (const auto& polyline : polylines) {
        const int thickness
            = static_cast<int>(std::lround(polyline.thickness.value_or(appearance.thickness)));
        for (const auto& s : mgo::levelformat::toSegments(polyline)) {
            shape.addShapeLine(
                { s.start.x,
                  s.start.y,
                  s.end.x,
                  s.end.y,
                  appearance.r,
                  appearance.g,
                  appearance.b,
                  255,
                  thickness });
        }
    }
}

// Text (the exit or fuel), centred at x, y
std::unique_ptr<GameShape> newText(
    GameShapeType type,
    const std::string& name,
    const std::string& text,
    double x,
    double y,
    std::optional<double> thickness,
    const Appearance& appearance)
{
    auto shape = newShape(type, name);
    shape->makeFromText(
        text,
        appearance.r,
        appearance.g,
        appearance.b,
        255,
        static_cast<int>(std::lround(thickness.value_or(appearance.thickness))));
    shape->setPos(x, y);
    return shape;
}

LevelData readJsonLevel(const std::string& contents)
{
    const mgo::levelformat::LevelData data = mgo::levelformat::parseJson(contents);
    LevelData level;
    level.description = data.title;
    if (data.ship.has_value()) {
        level.shipX = data.ship->x;
        level.shipY = data.ship->y;
        level.shipRotation = data.ship->rotation;
    }
    // In the same order as level_designer wrote the old format
    if (!data.walls.empty()) {
        auto walls = newShape(GameShapeType::OBSTRUCTION, "walls");
        addLines(*walls, data.walls, WALL);
        level.shapes.push_back(std::move(walls));
    }
    for (std::size_t i = 0; i < data.breakables.size(); ++i) {
        auto breakable = newShape(GameShapeType::BREAKABLE, std::format("breakable_{}", i));
        addLines(*breakable, data.breakables[i].lines, BREAKABLE);
        level.shapes.push_back(std::move(breakable));
    }
    if (data.exit.has_value()) {
        const auto& e = *data.exit;
        auto exit = newText(GameShapeType::EXIT, "exit", "EXIT", e.x, e.y, e.thickness, EXIT);
        // The format's rotations are anticlockwise; rotate()'s are clockwise
        exit->rotate(-e.rotation);
        level.shapes.push_back(std::move(exit));
    }
    for (std::size_t i = 0; i < data.fuel.size(); ++i) {
        const auto& f = data.fuel[i];
        level.shapes.push_back(newText(
            GameShapeType::FUEL, std::format("fuel_{}", i), "*", f.x, f.y, f.thickness, FUEL));
    }
    for (std::size_t i = 0; i < data.movingObjects.size(); ++i) {
        const auto& m = data.movingObjects[i];
        auto moving
            = newShape(GameShapeType::MOVING, m.name.value_or(std::format("moving_{}", i)));
        moving->setXDelta(static_cast<float>(m.speedX));
        moving->setXMaxDifference(static_cast<float>(m.rangeX));
        moving->setYDelta(static_cast<float>(m.speedY));
        moving->setYMaxDifference(static_cast<float>(m.rangeY));
        moving->setRotationDelta(static_cast<float>(m.spin));
        moving->setGravity(static_cast<float>(m.gravity));
        if (m.path.has_value()) {
            std::vector<std::pair<double, double>> offsets;
            for (const auto& offset : m.path->offsets) {
                offsets.emplace_back(offset.x, offset.y);
            }
            moving->setPath(std::move(offsets), m.path->speed, m.path->loop);
        }
        addLines(*moving, m.lines, MOVING);
        addObject(level, moving); // makes its lines relative to its centre
    }
    return level;
}

} // namespace

LevelData readLevelFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw AmazeRuntimeException("Failed to load Level file " + filename);
    }
    std::ostringstream contents;
    contents << file.rdbuf();

    // The formats are told apart by their first non-whitespace character:
    // '{' for JSON, '!' for the old format (see LEVEL_FORMAT.md)
    const std::string& text = contents.str();
    const auto first = std::ranges::find_if_not(
        text, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
    if (first != text.end() && *first == '{') {
        try {
            return readJsonLevel(text);
        } catch (const std::exception& e) {
            throw AmazeRuntimeException(
                std::format("Error in Level file {}: {}", filename, e.what()));
        }
    }

    // The old format
    std::istringstream in(text);
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
