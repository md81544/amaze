// Tests for the ship bouncing off obstacles, by running the GameModel's
// simulation in the same way as the Controller does.

#include "gamemodel.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <numbers>
#include <random>
#include <string>

using marengo::amaze::GameModel;
using marengo::amaze::GameShapeType;
using marengo::amaze::GameState;

namespace {

// An inside 90 degree corner at (100, 100): a wall going up from it, and a
// wall going right from it. The ship starts in the space between them.
constexpr double CORNER_X = 100.0;
constexpr double CORNER_Y = 100.0;
const std::string CORNER_LEVEL = "!~0~0~200~0~0~Corner\n"
                                 "N~OBSTRUCTION~walls\n"
                                 "L~100~-300~100~100~255~0~0~2\n"
                                 "L~100~100~500~100~255~0~0~2\n";

// Control values as sent by the keyboard handlers in GameShell
constexpr double ROTATION_PER_TICK = 0.75;
constexpr float THRUST = 6.f / 1000.f;

// A long horizontal wall along y = 100, and the same wall too far away to be
// reached, for comparison
const std::string WALL_LEVEL = "!~0~0~0~0~0~Wall\n"
                               "N~OBSTRUCTION~wall\n"
                               "L~-5000~100~5000~100~255~0~0~2\n";
const std::string NO_WALL_LEVEL = "!~0~0~0~0~0~No wall\n"
                                  "N~OBSTRUCTION~wall\n"
                                  "L~-5000~10000~5000~10000~255~0~0~2\n";

class TestLevel {
public:
    explicit TestLevel(const std::string& contents)
        : m_path(std::filesystem::temp_directory_path() / "amaze_bounce_test.cfg")
        , m_model(m_path.parent_path().string())
    {
        std::ofstream(m_path) << contents;
        m_model.levelLoad(m_path.string());
        m_model.setGameState(GameState::Running);
    }
    ~TestLevel()
    {
        std::filesystem::remove(m_path);
    }
    GameModel& model()
    {
        return m_model;
    }

private:
    std::filesystem::path m_path;
    GameModel m_model;
};

// One simulation tick, with the collision handling done by Controller::tick()
void tick(GameModel& model)
{
    model.tick();
    auto [collideeType, collider] = model.collisionDetect();
    if (collider && collideeType == GameShapeType::SHIP
        && collider->getGameShapeType() == GameShapeType::OBSTRUCTION) {
        model.bounceShip();
    }
}

bool shipTouchesWall(GameModel& model)
{
    auto [collideeType, collider] = model.collisionDetect();
    return collider && collideeType == GameShapeType::SHIP;
}

double distanceFromCorner(const GameModel& model)
{
    return std::hypot(model.shipModel().x() - CORNER_X, model.shipModel().y() - CORNER_Y);
}

// The ship's rotation which points its nose along (x, y). (The nose points
// along -(sine(rotation), cosine(rotation)).)
double rotationTowards(double x, double y)
{
    return std::atan2(-x, -y) * 180.0 / std::numbers::pi;
}

// Difference between two rotations, in the range -180..180
double angleDifference(double a, double b)
{
    return std::remainder(a - b, 360.0);
}

struct GrazeResult {
    double distance; // along the wall
    double speed; // along the wall, at the end
};

// Flies the ship along the wall, slightly towards it so that it grazes it
GrazeResult graze(const std::string& levelContents, int heading, double rotationDelta, bool thrust)
{
    TestLevel level(levelContents);
    GameModel& model = level.model();
    auto& ship = model.shipModel();
    ship.setRotation(heading);
    ship.setShipY(70.0);
    ship.setDx(-2.0);
    ship.setDy(-0.1);
    ship.setRotationDelta(rotationDelta);
    ship.setIsAccelerating(thrust, THRUST);
    for (int n = 0; n < 300; ++n) {
        tick(model);
    }
    return { ship.x(), -ship.dX() };
}

} // namespace

TEST_CASE("Ship keeps its momentum when grazing a wall")
{
    for (int heading = 0; heading < 360; heading += 15) {
        for (double rotationDelta : { 0.0, ROTATION_PER_TICK, -ROTATION_PER_TICK }) {
            INFO(std::format("heading {}, rotating {}", heading, rotationDelta));
            auto withWall = graze(WALL_LEVEL, heading, rotationDelta, false);
            auto withoutWall = graze(NO_WALL_LEVEL, heading, rotationDelta, false);
            CHECK(withWall.distance >= withoutWall.distance * 0.98);
            CHECK(withWall.speed >= withoutWall.speed * 0.98);
        }
    }
}

TEST_CASE("Ship keeps sliding along a wall when thrusting into it")
{
    // Nose pointing straight at the wall
    auto withWall = graze(WALL_LEVEL, 180, 0.0, true);
    auto withoutWall = graze(NO_WALL_LEVEL, 180, 0.0, true);
    // Being pressed against the wall slows it a bit, through friction
    CHECK(withWall.speed >= withoutWall.speed * 0.7);
    CHECK(withWall.distance >= withoutWall.distance * 0.8);
}

TEST_CASE("Ship can always get out of a corner it has been pushed into")
{
    for (int heading = 0; heading < 360; heading += 15) {
        for (double rotationDelta : { 0.0, ROTATION_PER_TICK, -ROTATION_PER_TICK }) {
            INFO(std::format("heading {}, rotating {}", heading, rotationDelta));
            TestLevel level(CORNER_LEVEL);
            GameModel& model = level.model();
            auto& ship = model.shipModel();
            ship.setRotation(heading);

            // Drift into the corner while thrusting and rotating, as a player
            // might when trying to get out of it
            ship.setDx(1.0);
            ship.setDy(-1.0);
            ship.setRotationDelta(rotationDelta);
            ship.setIsAccelerating(true, THRUST);
            for (int n = 0; n < 600; ++n) {
                tick(model);
                REQUIRE_FALSE(shipTouchesWall(model));
            }

            // Turn to face out of the corner, and fly out
            for (int n = 0; n < 2000 && distanceFromCorner(model) < 150.0; ++n) {
                double difference
                    = angleDifference(rotationTowards(1.0, -1.0), model.shipModel().rotation());
                model.shipModel().setRotationDelta(
                    std::clamp(difference, -ROTATION_PER_TICK, ROTATION_PER_TICK));
                model.shipModel().setIsAccelerating(std::abs(difference) < 10.0, THRUST);
                tick(model);
                REQUIRE_FALSE(shipTouchesWall(model));
            }
            CHECK(distanceFromCorner(model) >= 150.0);
        }
    }
}

TEST_CASE("Ship is never left touching a wall, whatever the player does")
{
    TestLevel level(CORNER_LEVEL);
    GameModel& model = level.model();
    auto& ship = model.shipModel();
    std::mt19937 random(1234); // fixed seed, so any failure is repeatable
    std::uniform_int_distribution<int> control(0, 5);
    for (int n = 0; n < 50000; ++n) {
        // Change controls every 30 ticks
        if (n % 30 == 0) {
            int c = control(random);
            ship.setRotationDelta(c == 0 ? ROTATION_PER_TICK : c == 1 ? -ROTATION_PER_TICK : 0.0);
            ship.setIsAccelerating(c >= 2, THRUST);
        }
        // Keep it near the corner
        if (distanceFromCorner(model) > 300.0) {
            ship.setShipX(200.0);
            ship.setShipY(0.0);
        }
        tick(model);
        INFO(std::format("tick {}", n));
        REQUIRE_FALSE(shipTouchesWall(model));
    }
}
