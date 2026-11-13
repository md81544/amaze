// Tests for reading level (.cfg) files

#include "exceptions.h"
#include "levelfile.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using marengo::amaze::AmazeRuntimeException;
using marengo::amaze::GameShapeType;
using marengo::amaze::readLevelFile;

namespace {

// Writes the contents to a temporary level file, removed when this goes out of scope
class TempLevelFile {
public:
    explicit TempLevelFile(const std::string& contents)
        : m_path(std::filesystem::temp_directory_path() / "amaze_levelfile_test.cfg")
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
        CHECK_THROWS_AS(readLevelFile("/nonexistent/level.cfg"), AmazeRuntimeException);
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
        if (entry.path().extension() == ".cfg") {
            INFO(entry.path().string());
            auto level = readLevelFile(entry.path().string());
            CHECK_FALSE(level.shapes.empty());
        }
    }
}
