#include "levelfile.h"
#include "exceptions.h"
#include "levelformat.h"

#include <cmath>
#include <format>
#include <fstream>
#include <sstream>

namespace marengo {
namespace amaze {

namespace {

// Adds the object to the level. A special object's lines are made relative to
// its centre.
void addObject(LevelData& level, std::unique_ptr<GameShape>& obj)
{
    if (obj->getGameShapeType() == GameShapeType::SPECIAL) {
        obj->setPosFromCentre();
    }
    level.shapes.push_back(std::move(obj));
}

// ---------------------------------------------------------------------------
// The JSON format (see LEVEL_FORMAT.md), read by levelformat.cpp. Each item
// becomes a GameShape, with its appearance (colours and default thicknesses) as given in
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
// Slightly lighter than the background grid (see GameModel::initialise())
constexpr Appearance DECORATION { 0, 90, 0, mgo::levelformat::DEFAULT_DECORATION_THICKNESS };
constexpr Appearance SPECIAL { 255, 172, 163, mgo::levelformat::DEFAULT_SPECIAL_OBJECT_THICKNESS };
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
    const mgo::levelformat::LevelData data = mgo::levelformat::parse(contents);
    LevelData level;
    level.description = data.title;
    if (data.ship.has_value()) {
        level.shipX = data.ship->x;
        level.shipY = data.ship->y;
        level.shipRotation = data.ship->rotation;
    }
    // Decorations first, so everything else is drawn over them. They're
    // NEUTRAL, like the background, so the ship passes over them.
    if (!data.decorations.empty()) {
        auto decorations = newShape(GameShapeType::NEUTRAL, "decorations");
        addLines(*decorations, data.decorations, DECORATION);
        level.shapes.push_back(std::move(decorations));
    }
    // Then the obstacles, exit and fuel, and lastly the special objects
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
    for (std::size_t i = 0; i < data.specialObjects.size(); ++i) {
        const auto& m = data.specialObjects[i];
        auto special
            = newShape(GameShapeType::SPECIAL, m.name.value_or(std::format("special_{}", i)));
        special->setXDelta(static_cast<float>(m.speedX));
        special->setXMaxDifference(static_cast<float>(m.rangeX));
        special->setYDelta(static_cast<float>(m.speedY));
        special->setYMaxDifference(static_cast<float>(m.rangeY));
        special->setRotationDelta(static_cast<float>(m.spin));
        special->setGravity(static_cast<float>(m.gravity));
        if (m.path.has_value()) {
            std::vector<std::pair<double, double>> offsets;
            for (const auto& offset : m.path->offsets) {
                offsets.emplace_back(offset.x, offset.y);
            }
            special->setPath(std::move(offsets), m.path->speed, m.path->loop);
        }
        addLines(*special, m.lines, SPECIAL);
        addObject(level, special); // makes its lines relative to its centre
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

    try {
        return readJsonLevel(contents.str());
    } catch (const std::exception& e) {
        throw AmazeRuntimeException(std::format("Error in Level file {}: {}", filename, e.what()));
    }
}

} // namespace amaze
} // namespace marengo
