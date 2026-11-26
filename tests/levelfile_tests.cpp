// Tests for reading level (.lvl) files

#include "exceptions.h"
#include "levelfile.h"
#include "levelfilelister.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

using marengo::amaze::AmazeRuntimeException;
using marengo::amaze::GameShape;
using marengo::amaze::GameShapeType;
using marengo::amaze::LevelData;
using marengo::amaze::readLevelFile;

namespace {

// Writes the contents to a temporary level file, removed when this goes out of scope
class TempLevelFile {
public:
    explicit TempLevelFile(const std::string& contents)
        : m_path(std::filesystem::temp_directory_path() / "amaze_levelfile_test.lvl")
    {
        std::ofstream(m_path) << contents;
    }
    ~TempLevelFile()
    {
        std::filesystem::remove(m_path);
    }
    std::string path() const
    {
        return m_path.string();
    }

private:
    std::filesystem::path m_path;
};

// A line of a shape, in arena coordinates, in a canonical form, for comparing
// shapes regardless of the order and direction of their lines
using LineKey = std::tuple<double, double, double, double, int, int, int, int, int>;

std::vector<LineKey> lineKeys(const GameShape& shape)
{
    // Rounded, as moving objects' lines are made relative to their centres
    auto round = [](double v) { return std::round(v * 1e6) / 1e6; };
    std::vector<LineKey> keys;
    for (const auto& l : shape.getVec()) {
        double x0 = round(l.x0 + shape.getPosX());
        double y0 = round(l.y0 + shape.getPosY());
        double x1 = round(l.x1 + shape.getPosX());
        double y1 = round(l.y1 + shape.getPosY());
        if (std::tie(x1, y1) < std::tie(x0, y0)) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        keys.emplace_back(x0, y0, x1, y1, l.r, l.g, l.b, l.a, l.lineThickness);
    }
    std::ranges::sort(keys);
    return keys;
}

} // namespace

TEST_CASE("The ship's position and the title are read")
{
    TempLevelFile file(R"({ "formatVersion": 1, "title": "Take it Slow",
                            "ship": { "x": 437, "y": 704, "rotation": 270 } })");
    auto level = readLevelFile(file.path());
    CHECK(level.shipX == 437);
    CHECK(level.shipY == 704);
    CHECK(level.shipRotation == 270);
    CHECK(level.description == "Take it Slow");
    CHECK(level.shapes.empty());
}

TEST_CASE("The title is optional")
{
    TempLevelFile file(R"({ "formatVersion": 1, "ship": { "x": 1, "y": 2 } })");
    CHECK(readLevelFile(file.path()).description.empty());
}

TEST_CASE("Each item becomes a shape, with the standard appearance")
{
    TempLevelFile file(R"({
      "formatVersion": 1,
      "exit": { "x": 1200, "y": 300 },
      "fuel": [ { "x": 100, "y": 200 }, { "x": 300, "y": 400, "thickness": 4 } ],
      "walls": [ { "points": [[400, 650], [450, 650], [450, 700]] }, { "points": [[0, 0], [5, 5]], "thickness": 1 } ],
      "breakables": [ { "lines": [ { "points": [[700, 550], [700, 650]] }, { "points": [[900, 550], [900, 650]] } ] } ],
      "movingObjects": [
        { "lines": [ { "points": [[0, 0], [10, 10]] } ], "name": "mover" }
      ]
    })");
    const LevelData level = readLevelFile(file.path());
    auto shapesOf = [&](GameShapeType type) {
        std::vector<const GameShape*> shapes;
        for (const auto& shape : level.shapes) {
            if (shape->getGameShapeType() == type) {
                shapes.push_back(shape.get());
            }
        }
        return shapes;
    };

    const auto walls = shapesOf(GameShapeType::OBSTRUCTION);
    REQUIRE(walls.size() == 1);
    CHECK(lineKeys(*walls[0])
          == std::vector<LineKey> { { 0, 0, 5, 5, 255, 0, 0, 255, 1 },
                                    { 400, 650, 450, 650, 255, 0, 0, 255, 2 },
                                    { 450, 650, 450, 700, 255, 0, 0, 255, 2 } });

    const auto breakables = shapesOf(GameShapeType::BREAKABLE);
    REQUIRE(breakables.size() == 1); // breaks as a whole
    CHECK(lineKeys(*breakables[0])
          == std::vector<LineKey> { { 700, 550, 700, 650, 255, 150, 50, 255, 6 },
                                    { 900, 550, 900, 650, 255, 150, 50, 255, 6 } });

    const auto exits = shapesOf(GameShapeType::EXIT);
    REQUIRE(exits.size() == 1);
    CHECK(exits[0]->getPosX() == 1200);
    CHECK(exits[0]->getPosY() == 300);
    REQUIRE_FALSE(exits[0]->getVec().empty());
    CHECK(exits[0]->getVec()[0].lineThickness == 6);
    CHECK(exits[0]->getVec()[0].r == 52);

    const auto fuel = shapesOf(GameShapeType::FUEL);
    REQUIRE(fuel.size() == 2);
    CHECK(fuel[0]->getPosX() == 100);
    CHECK(fuel[0]->getPosY() == 200);
    REQUIRE_FALSE(fuel[0]->getVec().empty());
    CHECK(fuel[0]->getVec()[0].lineThickness == 12);
    REQUIRE_FALSE(fuel[1]->getVec().empty());
    CHECK(fuel[1]->getVec()[0].lineThickness == 4);

    const auto moving = shapesOf(GameShapeType::MOVING);
    REQUIRE(moving.size() == 1);
    CHECK(moving[0]->getName() == "mover");
    CHECK(moving[0]->getPosX() == 5); // its lines are relative to its centre
    CHECK(moving[0]->getPosY() == 5);
    CHECK(lineKeys(*moving[0])
          == std::vector<LineKey> { { 0, 0, 10, 10, 255, 172, 163, 255, 6 } });
}

TEST_CASE("Moving objects get their motion parameters")
{
    TempLevelFile file(R"({
      "formatVersion": 1,
      "movingObjects": [
        { "lines": [ { "points": [[0, 0], [10, 10]] } ], "speedX": 1.5, "rangeX": 100, "speedY": -2,
          "rangeY": 50, "spin": 0.5, "gravity": 3, "name": "mover" }
      ]
    })");
    auto level = readLevelFile(file.path());
    REQUIRE(level.shapes.size() == 1);
    const auto& mover = *level.shapes[0];
    CHECK(mover.getGameShapeType() == GameShapeType::MOVING);
    CHECK(mover.getName() == "mover");
    CHECK(mover.getXDelta() == 1.5f);
    CHECK(mover.getXMaxDifference() == 100.f);
    CHECK(mover.getYDelta() == -2.f);
    CHECK(mover.getYMaxDifference() == 50.f);
    CHECK(mover.getRotationDelta() == 0.5f);
    CHECK(mover.getGravity() == 3.f);
}

TEST_CASE("Moving objects get their paths")
{
    TempLevelFile file(R"({
      "formatVersion": 1,
      "movingObjects": [
        { "lines": [ { "points": [[0, 0], [10, 10]] } ],
          "path": { "offsets": [[100, -50], [0, 200]], "speed": 0.75, "loop": true } },
        { "lines": [ { "points": [[0, 0], [10, 10]] } ], "speedX": 1, "rangeX": 10 }
      ]
    })");
    auto level = readLevelFile(file.path());
    REQUIRE(level.shapes.size() == 2);
    const auto& onPath = *level.shapes[0];
    CHECK(onPath.hasPath());
    CHECK(onPath.getPathOffsets() == std::vector<std::pair<double, double>> { { 100, -50 }, { 0, 200 } });
    CHECK(onPath.getPathSpeed() == 0.75);
    CHECK(onPath.getPathLoops());
    CHECK_FALSE(level.shapes[1]->hasPath());
}

TEST_CASE("Bad level files throw, saying what's wrong")
{
    auto error = [](const std::string& filename) {
        try {
            readLevelFile(filename);
        } catch (const AmazeRuntimeException& e) {
            return std::string(e.what());
        }
        return std::string("(no error)");
    };
    CHECK(error("/nonexistent/level.lvl") == "Failed to load Level file /nonexistent/level.lvl");
    {
        TempLevelFile file(R"({ "formatVersion": 1, "ship": { "x": 1 } })");
        CHECK(error(file.path()).ends_with("ship: missing \"y\""));
    }
    {
        TempLevelFile file("!~0~0~1~2~3~Title\nN~OBSTRUCTION~wall\n");
        CHECK(error(file.path()).ends_with("the old format, which is no longer supported"));
    }
    {
        TempLevelFile file("");
        CHECK(error(file.path()).ends_with("This isn't a level file (or it's empty)"));
    }
}

TEST_CASE("All shipped levels load")
{
    for (const auto& entry : std::filesystem::directory_iterator(AMAZE_DATA_DIR)) {
        if (entry.path().extension() == ".lvl") {
            INFO(entry.path().string());
            auto level = readLevelFile(entry.path().string());
            CHECK_FALSE(level.shapes.empty());
        }
    }
}

TEST_CASE("Decorations are drawn, but aren't obstacles")
{
    TempLevelFile file(R"({
      "formatVersion": 1,
      "decorations": [ { "points": [[100, 100], [200, 100], [200, 200]] }, { "points": [[0, 0], [5, 5]], "thickness": 4 } ]
    })");
    const LevelData level = readLevelFile(file.path());
    REQUIRE(level.shapes.size() == 1);
    const auto& decorations = *level.shapes[0];
    CHECK(decorations.getGameShapeType() == GameShapeType::NEUTRAL);
    CHECK(decorations.getName() == "decorations");
    CHECK(lineKeys(decorations)
          == std::vector<LineKey> { { 0, 0, 5, 5, 0, 90, 0, 255, 4 },
                                    { 100, 100, 200, 100, 0, 90, 0, 255, 2 },
                                    { 200, 100, 200, 200, 0, 90, 0, 255, 2 } });
}

TEST_CASE("The exit's rotation is applied anticlockwise, as level_designer shows it")
{
    TempLevelFile file(R"({ "formatVersion": 1, "exit": { "x": 100, "y": 200, "rotation": 90 } })");
    const LevelData level = readLevelFile(file.path());
    REQUIRE(level.shapes.size() == 1);
    const auto& exit = *level.shapes[0];
    CHECK(exit.getGameShapeType() == GameShapeType::EXIT);
    CHECK(exit.getPosX() == 100);
    CHECK(exit.getPosY() == 200);
    // Upright, the text is wider than it's tall; turned anticlockwise by 90
    // degrees (on screen, with y down), what was the right end of the text
    // (x > 0) is now at the top (y < 0)
    GameShape upright;
    upright.makeFromText("EXIT", 52, 213, 235, 255, 6);
    REQUIRE(upright.getVec().size() == exit.getVec().size());
    for (std::size_t i = 0; i < exit.getVec().size(); ++i) {
        const auto& before = upright.getVec()[i];
        const auto& after = exit.getVec()[i];
        CHECK(std::abs(after.x0 - before.y0) < 1e-9);
        CHECK(std::abs(after.y0 + before.x0) < 1e-9);
    }
}

TEST_CASE("The level list shows each level's title")
{
    marengo::amaze::LevelFileLister lister(AMAZE_DATA_DIR);
    const auto files = lister.getFileMap();
    REQUIRE_FALSE(files.empty());
    for (const auto& [number, file] : files) {
        INFO(file.filename);
        CHECK(file.description == readLevelFile(file.filename).description);
        CHECK(file.description != "Untitled");
    }
    CHECK(files.at(0).description == "Rookie Trainer");
}
