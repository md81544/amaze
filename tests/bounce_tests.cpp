// Tests for the ship bouncing off obstacles, by running the GameModel's
// simulation in the same way as the Controller does.

#include "gamemodel.h"

#include <catch2/catch_approx.hpp>
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
const std::string CORNER_LEVEL = R"({ "formatVersion": 2, "ship": { "x": 200, "y": 0 },
  "walls": [ { "points": [[100, -300], [100, 100], [500, 100]] } ] })";

// Control values as sent by the keyboard handlers in GameShell
constexpr double ROTATION_PER_TICK = 0.75;
constexpr float THRUST = 6.f / 1000.f;

// A long horizontal wall along y = 100, and the same wall too far away to be
// reached, for comparison
const std::string WALL_LEVEL = R"({ "formatVersion": 2, "ship": { "x": 0, "y": 0 },
  "walls": [ { "points": [[-5000, 100], [5000, 100]] } ] })";
const std::string NO_WALL_LEVEL = R"({ "formatVersion": 2, "ship": { "x": 0, "y": 0 },
  "walls": [ { "points": [[-5000, 10000], [5000, 10000]] } ] })";

class TestLevel {
public:
    explicit TestLevel(const std::string& contents)
        : m_path(std::filesystem::temp_directory_path() / "amaze_bounce_test.lvl")
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
        model.shieldImpact(model.bounceShip());
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

namespace {

struct DropResult {
    double upwardSpeed; // after bouncing
    double rotation;
    bool rightWingRaised; // relative to the left wing
};

// Drops the upright ship, tail first, at 1 unit per tick from (0, 60) onto
// the level's obstacles, and lets it bounce
DropResult dropShip(const std::string& levelContents)
{
    TestLevel level(levelContents);
    GameModel& model = level.model();
    auto& ship = model.shipModel();
    ship.setShipY(60.0);
    ship.setDy(-1.0);
    for (int n = 0; n < 60; ++n) {
        tick(model);
    }
    // The ends of the wings (see ShipModel's constructor)
    const auto& lines = ship.shipGameShape()->getVec();
    double rightWingY = lines[3].y1;
    double leftWingY = lines[11].y1;
    return { ship.dY(), ship.rotation(), rightWingY < leftWingY - 0.01 };
}

} // namespace

TEST_CASE("Ship hitting a wall square on bounces without spinning")
{
    auto result = dropShip(WALL_LEVEL);
    CHECK(result.upwardSpeed > 0.0);
    CHECK(result.rotation == Catch::Approx(0.0).margin(1e-6));
}

TEST_CASE("Ship hitting a corner with a wing spins away from it")
{
    // An outside corner at (8, 100), under the ship's right wing
    const std::string cornerUnderWing = R"({ "formatVersion": 2, "ship": { "x": 0, "y": 0 },
      "walls": [ { "points": [[300, 100], [8, 100], [8, 300]] } ] })";
    auto wingHit = dropShip(cornerUnderWing);
    auto squareHit = dropShip(WALL_LEVEL);
    CHECK(wingHit.rightWingRaised);
    CHECK(std::abs(wingHit.rotation) > 1.0);
    // Some of the impact went into spinning it rather than bouncing it
    CHECK(wingHit.upwardSpeed > 0.0);
    CHECK(wingHit.upwardSpeed < squareHit.upwardSpeed);
}

TEST_CASE("Shield damage depends on the speed of impact")
{
    TestLevel level(WALL_LEVEL);
    GameModel& model = level.model();
    CHECK(model.shield() == 1.0);

    SECTION("Very slow impacts, e.g. thrusting against a wall, do no damage")
    {
        model.shieldImpact({ 0.01 });
        CHECK(model.shield() == 1.0);
    }
    SECTION("A graze takes 10%")
    {
        model.shieldImpact({ 0.02 });
        CHECK(model.shield() == Catch::Approx(0.9));
    }
    SECTION("Damage increases with speed")
    {
        model.shieldImpact({ 0.16 });
        CHECK(model.shield() == Catch::Approx(0.45));
    }
    SECTION("A major impact takes all of it")
    {
        model.shieldImpact({ 0.3 });
        CHECK(model.shield() == 0.0);
        model.shieldImpact({ 2.0 });
        CHECK(model.shield() == 0.0);
    }
}

TEST_CASE("Impacts in quick succession only weaken the shield once")
{
    TestLevel level(WALL_LEVEL);
    GameModel& model = level.model();
    // e.g. bouncing a few times as the ship comes to rest
    model.shieldImpact({ 0.16 });
    model.tick();
    model.shieldImpact({ 0.09 });
    model.tick();
    model.shieldImpact({ 0.04 });
    CHECK(model.shield() == Catch::Approx(0.45));

    // A harder impact soon after only takes the extra damage
    model.tick();
    model.shieldImpact({ 0.23 });
    CHECK(model.shield() == Catch::Approx(0.225));

    // Once things have settled, another impact counts in full
    for (int n = 0; n < 60; ++n) {
        model.tick();
    }
    model.shieldImpact({ 0.02 });
    CHECK(model.shield() == Catch::Approx(0.125));
}

TEST_CASE("Shield recharges up to full")
{
    TestLevel level(WALL_LEVEL);
    GameModel& model = level.model();
    model.shieldImpact({ 0.02 });
    model.rechargeShield(0.1);
    CHECK(model.shield() == Catch::Approx(1.0));
    model.rechargeShield(0.1);
    CHECK(model.shield() == 1.0);
}

TEST_CASE("Thrusting against a wall doesn't drain the shield")
{
    TestLevel level(WALL_LEVEL);
    GameModel& model = level.model();
    auto& ship = model.shipModel();
    // Just above the wall, with the nose pointing at it
    ship.setRotation(180);
    ship.setShipY(78.0);
    ship.setIsAccelerating(true, THRUST);
    auto run = [&](int ticks) {
        for (int n = 0; n < ticks; ++n) {
            tick(model);
        }
    };
    // The first impact weakens the shield
    run(240);
    double shieldAfterImpact = model.shield();
    CHECK(shieldAfterImpact < 1.0);
    CHECK(shieldAfterImpact > 0.0);
    // But staying pressed against the wall doesn't
    run(1200);
    CHECK(model.shield() == shieldAfterImpact);
}

TEST_CASE("Explosions aren't collided with, even where they were last shown")
{
    TestLevel level(WALL_LEVEL);
    GameModel& model = level.model();
    // A breakable object explodes at the ship's position, which isn't moving
    model.setBreakableExploding();
    for (int n = 0; n < 100; ++n) {
        model.tick();
    }
    auto [collideeType, collider] = model.collisionDetect();
    CHECK(collider == nullptr);
}

TEST_CASE("Rotating into a wall counts as a graze")
{
    TestLevel level(WALL_LEVEL);
    GameModel& model = level.model();
    auto& ship = model.shipModel();
    // Stationary, with the wings just above the wall
    ship.setShipY(80.5);
    ship.setRotationDelta(ROTATION_PER_TICK);
    for (int n = 0; n < 30; ++n) {
        tick(model);
    }
    CHECK(model.shield() == Catch::Approx(0.9));

    // Carrying on rotating against it is the same graze
    for (int n = 0; n < 600; ++n) {
        tick(model);
    }
    CHECK(model.shield() == Catch::Approx(0.9));
}

TEST_CASE("Fuel is only collided with when the shield needs recharging")
{
    // Fuel across the ship's starting position
    TestLevel level(R"({ "formatVersion": 2, "ship": { "x": 0, "y": 0 },
                         "fuel": [ { "x": 0, "y": 0 } ] })");
    GameModel& model = level.model();
    auto [fullType, fullCollider] = model.collisionDetect();
    CHECK(fullCollider == nullptr);

    model.shieldImpact({ 0.02 });
    auto [weakType, weakCollider] = model.collisionDetect();
    REQUIRE(weakCollider != nullptr);
    CHECK(weakCollider->getGameShapeType() == GameShapeType::FUEL);
}
