// Tests for moving objects following paths (see LEVEL_FORMAT.md)

#include "gameshape.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <utility>
#include <vector>

using Catch::Approx;
using marengo::amaze::GameShape;

namespace {

// A shape at (100, 100), following the path
GameShape onPath(std::vector<std::pair<double, double>> offsets, double speed, bool loop)
{
    GameShape shape;
    shape.setPos(100, 100);
    shape.setPath(std::move(offsets), speed, loop);
    return shape;
}

void moveTimes(GameShape& shape, int ticks)
{
    for (int i = 0; i < ticks; ++i) {
        shape.move();
    }
}

} // namespace

TEST_CASE("A shape goes back and forth along a diagonal path")
{
    // 50 long
    GameShape shape = onPath({ { 30, 40 } }, 1, false);
    CHECK(shape.hasPath());
    moveTimes(shape, 10);
    CHECK(shape.getPosX() == Approx(106));
    CHECK(shape.getPosY() == Approx(108));
    moveTimes(shape, 40);
    CHECK(shape.getPosX() == Approx(130));
    CHECK(shape.getPosY() == Approx(140));
    moveTimes(shape, 10); // on its way back
    CHECK(shape.getPosX() == Approx(124));
    CHECK(shape.getPosY() == Approx(132));
    moveTimes(shape, 40);
    CHECK(shape.getPosX() == Approx(100));
    CHECK(shape.getPosY() == Approx(100));
    moveTimes(shape, 10); // and out again
    CHECK(shape.getPosX() == Approx(106));
    CHECK(shape.getPosY() == Approx(108));
}

TEST_CASE("A shape turns round at the ends of its path by however far it overshot")
{
    GameShape shape = onPath({ { 10, 0 } }, 3, false);
    std::vector<double> xs;
    for (int i = 0; i < 9; ++i) {
        shape.move();
        xs.push_back(shape.getPosX() - 100);
    }
    const std::vector<double> expected { 3, 6, 9, 8, 5, 2, 1, 4, 7 };
    REQUIRE(xs.size() == expected.size());
    for (std::size_t i = 0; i < xs.size(); ++i) {
        CHECK(xs[i] == Approx(expected[i]));
    }
}

TEST_CASE("A shape never drifts off its path")
{
    // Speeds which don't divide the path's length exactly, over many trips
    GameShape shape = onPath({ { 173, -97 } }, 0.37, false);
    const double length = std::hypot(173, 97);
    for (int i = 0; i < 100000; ++i) {
        shape.move();
        const double dx = shape.getPosX() - 100;
        const double dy = shape.getPosY() - 100;
        // On the line, and within its ends
        REQUIRE(std::abs(dx * 97 + dy * 173) < 1e-6);
        REQUIRE(std::hypot(dx, dy) <= length + 1e-9);
        REQUIRE(dx >= -1e-9);
    }
}

TEST_CASE("A shape follows a path with several points, coming back the same way")
{
    GameShape shape = onPath({ { 100, 0 }, { 100, 100 } }, 10, false);
    moveTimes(shape, 15);
    CHECK(shape.getPosX() == Approx(200));
    CHECK(shape.getPosY() == Approx(150));
    moveTimes(shape, 10); // to the end (at 20 ticks) and back along the second line
    CHECK(shape.getPosX() == Approx(200));
    CHECK(shape.getPosY() == Approx(150));
    moveTimes(shape, 10);
    CHECK(shape.getPosX() == Approx(150));
    CHECK(shape.getPosY() == Approx(100));
}

TEST_CASE("A shape on a looping path goes straight back to the start, and round again")
{
    // Round a right-angled triangle: 100 across, 100 down, then diagonally back
    GameShape shape = onPath({ { 100, 0 }, { 100, 100 } }, 10, true);
    const double length = 200 + std::hypot(100, 100);
    moveTimes(shape, 25); // 50 along the diagonal
    const double along = 50 / std::hypot(100, 100);
    CHECK(shape.getPosX() == Approx(200 - 100 * along));
    CHECK(shape.getPosY() == Approx(200 - 100 * along));
    moveTimes(shape, 10); // past the start, onto the first line again
    CHECK(shape.getPosX() == Approx(100 + (350 - length)));
    CHECK(shape.getPosY() == Approx(100));
}

TEST_CASE("A shape with a path still spins")
{
    GameShape shape = onPath({ { 10, 0 } }, 1, false);
    shape.setRotationDelta(90);
    shape.addLine(10, 0, 20, 0);
    shape.move();
    const auto& line = shape.getVec().at(0);
    CHECK(line.x0 == Approx(0).margin(1e-9));
    CHECK(line.y0 == Approx(10));
    CHECK(shape.getPosX() == Approx(101));
}

TEST_CASE("A path with no length leaves the shape where it is")
{
    GameShape shape = onPath({ { 0, 0 } }, 5, false);
    moveTimes(shape, 3);
    CHECK(shape.getPosX() == 100);
    CHECK(shape.getPosY() == 100);
}
