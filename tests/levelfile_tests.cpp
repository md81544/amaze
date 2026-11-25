// Tests for reading level (.lvl) files

#include "exceptions.h"
#include "levelfile.h"
#include "levelfilelister.h"
#include "levelformat.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
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

// What matters about a level to the game, regardless of the order of its
// shapes and lines, and of how walls are split into obstruction shapes
struct Comparable {
    double shipX;
    double shipY;
    double shipRotation;
    std::string description;
    std::vector<LineKey> walls;
    // For other shapes: their type, motion (for moving objects) and lines
    std::vector<std::tuple<GameShapeType, std::vector<float>, std::vector<LineKey>>> others;
    bool operator==(const Comparable&) const = default;
};

Comparable comparable(const LevelData& level)
{
    Comparable c { level.shipX, level.shipY, level.shipRotation, level.description, {}, {} };
    for (const auto& shape : level.shapes) {
        const auto keys = lineKeys(*shape);
        if (shape->getGameShapeType() == GameShapeType::OBSTRUCTION) {
            c.walls.insert(c.walls.end(), keys.begin(), keys.end());
            continue;
        }
        std::vector<float> motion;
        if (shape->getGameShapeType() == GameShapeType::MOVING) {
            motion = { shape->getXDelta(),          shape->getXMaxDifference(),
                       shape->getYDelta(),          shape->getYMaxDifference(),
                       shape->getRotationDelta(), shape->getGravity() };
        }
        c.others.emplace_back(shape->getGameShapeType(), motion, keys);
    }
    std::ranges::sort(c.walls);
    std::ranges::sort(c.others);
    return c;
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

} // namespace

TEST_CASE("Header sets ship position and description")
{
    TempLevelFile file("!~0~0~437~704~270~Take it Slow\n");
    auto level = readLevelFile(file.path());
    CHECK(level.shipX == 437);
    CHECK(level.shipY == 704);
    CHECK(level.shipRotation == 270);
    CHECK(level.description == "Take it Slow");
    CHECK(level.shapes.empty());
}

TEST_CASE("Description is optional")
{
    TempLevelFile file("!~0~0~1~2~3\n");
    CHECK(readLevelFile(file.path()).description.empty());
}

TEST_CASE("Objects get their type, lines and position")
{
    TempLevelFile file(
        "!~0~0~1~2~3~Test\n"
        "N~OBSTRUCTION~wall\n"
        "L~400~650~450~650~255~0~0~2\n"
        "L~450~650~450~700~255~0~0\n"
        "N~FUEL~fuel\n"
        "P~100~200\n"
        "N~SOMETHINGELSE\n");
    auto level = readLevelFile(file.path());
    REQUIRE(level.shapes.size() == 3);

    const auto& wall = *level.shapes[0];
    CHECK(wall.getGameShapeType() == GameShapeType::OBSTRUCTION);
    REQUIRE(wall.getVec().size() == 2);
    CHECK(wall.getVec()[0].lineThickness == 2);
    CHECK(wall.getVec()[1].lineThickness == 1); // default

    const auto& fuel = *level.shapes[1];
    CHECK(fuel.getGameShapeType() == GameShapeType::FUEL);
    CHECK(fuel.getPosX() == 100);
    CHECK(fuel.getPosY() == 200);

    CHECK(level.shapes[2]->getGameShapeType() == GameShapeType::NEUTRAL);
}

TEST_CASE("Moving objects get their motion parameters")
{
    TempLevelFile file(
        "!~0~0~1~2~3\n"
        "N~MOVING~mover~1.5~100~-2~50~0.5~3\n"
        "L~0~0~10~10~255~255~255\n");
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

TEST_CASE("Lines before the first object are ignored")
{
    TempLevelFile file(
        "!~0~0~1~2~3\n"
        "L~0~0~10~10~255~255~255\n");
    CHECK(readLevelFile(file.path()).shapes.empty());
}

TEST_CASE("Bad level files throw")
{
    SECTION("missing file")
    {
        CHECK_THROWS_AS(readLevelFile("/nonexistent/level.lvl"), AmazeRuntimeException);
    }
    SECTION("missing header")
    {
        TempLevelFile file("N~OBSTRUCTION~wall\n");
        CHECK_THROWS_AS(readLevelFile(file.path()), AmazeRuntimeException);
    }
    SECTION("short header")
    {
        TempLevelFile file("!~0~0~1\n");
        CHECK_THROWS_AS(readLevelFile(file.path()), AmazeRuntimeException);
    }
    SECTION("non-numeric value")
    {
        TempLevelFile file("!~0~0~1~2~3\nN~OBSTRUCTION~wall\nP~abc~200\n");
        CHECK_THROWS_AS(readLevelFile(file.path()), AmazeRuntimeException);
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

TEST_CASE("JSON levels give the same shapes as old format levels")
{
    TempLevelFile file(R"({
      "formatVersion": 1,
      "title": "Test",
      "ship": { "x": 1, "y": 2, "rotation": 3 },
      "exit": { "x": 1200, "y": 300 },
      "fuel": [ { "x": 100, "y": 200 }, { "x": 300, "y": 400, "thickness": 4 } ],
      "walls": [ { "points": [[400, 650], [450, 650], [450, 700]] }, { "points": [[0, 0], [5, 5]], "thickness": 1 } ],
      "breakables": [ { "lines": [ { "points": [[700, 550], [700, 650]] }, { "points": [[900, 550], [900, 650]] } ] } ],
      "movingObjects": [
        { "lines": [ { "points": [[0, 0], [10, 10]] } ], "speedX": 1.5, "rangeX": 100, "speedY": -2,
          "rangeY": 50, "spin": 0.5, "gravity": 3, "name": "mover" }
      ]
    })");
    TempLevelFile old("!~0~0~1~2~3~Test\n"
                      "N~OBSTRUCTION~obstruction\n"
                      "L~400~650~450~650~255~0~0~2\n"
                      "L~450~650~450~700~255~0~0~2\n"
                      "L~0~0~5~5~255~0~0\n" // thickness 1 when not given
                      "N~BREAKABLE~breakable\n"
                      "L~700~550~700~650~255~150~50~6\n"
                      "L~900~550~900~650~255~150~50~6\n"
                      "N~EXIT~exit\n"
                      "T~EXIT~52~213~235~6\n"
                      "P~1200~300\n"
                      "N~FUEL~fuel\n"
                      "T~*~255~255~0~12\n"
                      "P~100~200\n"
                      "N~FUEL~fuel\n"
                      "T~*~255~255~0~4\n"
                      "P~300~400\n"
                      "N~MOVING~mover~1.5~100~-2~50~0.5~3\n"
                      "L~0~0~10~10~255~172~163~6\n");
    // The temporary files share a name, so read each before writing the next
    const LevelData fromJson = readLevelFile(file.path());
    const LevelData fromOld = [&] {
        TempLevelFile oldFile(readFile(old.path()));
        return readLevelFile(oldFile.path());
    }();
    CHECK(comparable(fromJson) == comparable(fromOld));

    const auto& mover = *fromJson.shapes.back();
    REQUIRE(mover.getGameShapeType() == GameShapeType::MOVING);
    CHECK(mover.getName() == "mover");
    CHECK(mover.getPosX() == 5); // its lines are relative to its centre
    CHECK(mover.getPosY() == 5);
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

TEST_CASE("Bad JSON level files throw")
{
    TempLevelFile file(R"({ "formatVersion": 1, "ship": { "x": 1 } })");
    CHECK_THROWS_AS(readLevelFile(file.path()), AmazeRuntimeException);
}

TEST_CASE("Every old format level gives the same shapes when converted to JSON")
{
    // Converted by levelformat.cpp, as level_designer does
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(AMAZE_DATA_DIR)) {
        const std::string contents = readFile(entry.path());
        if (entry.path().extension() != ".lvl" || !contents.starts_with('!')) {
            continue;
        }
        INFO(entry.path().string());
        ++count;
        const LevelData old = readLevelFile(entry.path().string());
        TempLevelFile json(mgo::levelformat::toJson(mgo::levelformat::parseOldFormat(contents)));
        CHECK(comparable(readLevelFile(json.path())) == comparable(old));
    }
    if (count == 0) {
        SKIP("No old format levels left to check, in " << AMAZE_DATA_DIR);
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
