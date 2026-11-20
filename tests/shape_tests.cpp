// Tests for Shape's geometry: collision detection, rotation and resizing.

#include "shape.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <initializer_list>
#include <memory>

using marengo::amaze::Shape;
using marengo::amaze::ShapeLine;

namespace {

// Makes a shape from lines given as {x0, y0, x1, y1}, at the given position
std::shared_ptr<Shape>
makeShape(std::initializer_list<std::array<double, 4>> lines, double x = 0.0, double y = 0.0)
{
    auto shape = std::make_shared<Shape>();
    for (const auto& l : lines) {
        shape->addShapeLine(ShapeLine { l[0], l[1], l[2], l[3], 255, 255, 255, 255, 1 });
    }
    shape->setPos(x, y);
    return shape;
}

bool collide(const std::shared_ptr<Shape>& a, const std::shared_ptr<Shape>& b)
{
    // Collision should be symmetric, so check both ways
    bool ab = a->intersectCheck(b);
    bool ba = b->intersectCheck(a);
    CHECK(ab == ba);
    return ab;
}

} // namespace

TEST_CASE("Crossing lines collide")
{
    CHECK(collide(makeShape({ { 0, 0, 10, 10 } }), makeShape({ { 0, 10, 10, 0 } })));
    CHECK(collide(makeShape({ { 0, 5, 10, 5 } }), makeShape({ { 5, 0, 5, 10 } })));
    // Non-integer coordinates
    CHECK(collide(makeShape({ { 0.5, 0.5, 9.5, 9.5 } }), makeShape({ { 0.5, 9.5, 9.5, 0.5 } })));
}

TEST_CASE("Separate lines don't collide")
{
    // Parallel
    CHECK_FALSE(collide(makeShape({ { 0, 0, 10, 0 } }), makeShape({ { 0, 1, 10, 1 } })));
    // Far apart
    CHECK_FALSE(collide(makeShape({ { 0, 0, 10, 10 } }), makeShape({ { 100, 0, 110, 10 } })));
    // Would cross if either were extended
    CHECK_FALSE(collide(makeShape({ { 0, 0, 10, 0 } }), makeShape({ { 15, -5, 15, 5 } })));
    CHECK_FALSE(collide(makeShape({ { 0, 0, 4, 4 } }), makeShape({ { 0, 10, 10, 0 } })));
}

TEST_CASE("Touching lines collide")
{
    // T junction: one line ends on the other
    CHECK(collide(makeShape({ { 0, 0, 10, 0 } }), makeShape({ { 5, 0, 5, 10 } })));
    // Shared end point
    CHECK(collide(makeShape({ { 0, 0, 10, 0 } }), makeShape({ { 10, 0, 10, 10 } })));
}

TEST_CASE("Collinear lines collide only if they overlap")
{
    CHECK(collide(makeShape({ { 0, 0, 10, 0 } }), makeShape({ { 5, 0, 15, 0 } })));
    CHECK(collide(makeShape({ { 0, 0, 10, 10 } }), makeShape({ { 2, 2, 4, 4 } })));
    CHECK(collide(makeShape({ { 0, 0, 0, 10 } }), makeShape({ { 0, 10, 0, 20 } })));
    CHECK_FALSE(collide(makeShape({ { 0, 0, 10, 0 } }), makeShape({ { 11, 0, 20, 0 } })));
}

TEST_CASE("Zero-length lines (points) collide only if on the other line")
{
    // Level files contain lines like L~700~650~700~650
    CHECK(collide(makeShape({ { 5, 0, 5, 0 } }), makeShape({ { 0, 0, 10, 0 } })));
    CHECK(collide(makeShape({ { 0, 0, 0, 0 } }), makeShape({ { 0, 0, 10, 0 } })));
    CHECK_FALSE(collide(makeShape({ { 5, 1, 5, 1 } }), makeShape({ { 0, 0, 10, 0 } })));
    CHECK_FALSE(collide(makeShape({ { 15, 0, 15, 0 } }), makeShape({ { 0, 0, 10, 0 } })));
}

TEST_CASE("Shape positions are taken into account")
{
    auto horizontal = makeShape({ { -10, 0, 10, 0 } }, 100, 100);
    auto vertical = makeShape({ { 0, -10, 0, 10 } });

    vertical->setPos(105, 100);
    CHECK(collide(horizontal, vertical));
    vertical->setPos(105, 115);
    CHECK_FALSE(collide(horizontal, vertical));
    vertical->setPos(125, 100);
    CHECK_FALSE(collide(horizontal, vertical));
}

TEST_CASE("Any pair of lines in multi-line shapes can collide")
{
    // A square, and a wall which only one side of the square crosses
    auto square
        = makeShape({ { 0, 0, 10, 0 }, { 10, 0, 10, 10 }, { 10, 10, 0, 10 }, { 0, 10, 0, 0 } });
    auto wall = makeShape({ { 50, 0, 60, 0 }, { 8, -5, 12, -5 }, { 5, 5, 20, 5 } });
    CHECK(collide(square, wall));
    wall->setPos(20, 0);
    CHECK_FALSE(collide(square, wall));
}

TEST_CASE("Rotation turns a shape about its origin")
{
    auto shape = makeShape({ { 10, 0, 20, 0 } });
    shape->rotate(90);
    // Positive angles turn clockwise on screen (y down): (10, 0) -> (0, 10)
    const auto& line = shape->getVec().at(0);
    CHECK(line.x0 == Catch::Approx(0).margin(1e-9));
    CHECK(line.y0 == Catch::Approx(10));
    CHECK(line.x1 == Catch::Approx(0).margin(1e-9));
    CHECK(line.y1 == Catch::Approx(20));

    // Rotations accumulate
    shape->rotate(45);
    shape->rotate(45);
    const auto& turned = shape->getVec().at(0);
    CHECK(turned.x0 == Catch::Approx(-10));
    CHECK(turned.y0 == Catch::Approx(0).margin(1e-9));
}

TEST_CASE("Rotation affects collisions")
{
    auto spinner = makeShape({ { -10, 0, 10, 0 } }, 50, 50);
    auto probe = makeShape({ { -1, 0, 1, 0 } }, 50, 55);
    CHECK_FALSE(collide(spinner, probe));
    spinner->rotate(90);
    CHECK(collide(spinner, probe));
    spinner->rotate(90);
    CHECK_FALSE(collide(spinner, probe));
}

TEST_CASE("Resizing scales a shape about its origin, cumulatively")
{
    auto shape = makeShape({ { 0, 0, 10, 0 } });
    auto probe = makeShape({ { 15, -5, 15, 5 } });
    CHECK_FALSE(collide(shape, probe));

    shape->resize(2);
    CHECK(shape->getVec().at(0).x1 == Catch::Approx(20));
    CHECK(collide(shape, probe));

    // Factors multiply, e.g. the explosion shapes start at 0.1 and grow by
    // 1.2 each tick
    shape->resize(0.5);
    CHECK(shape->getVec().at(0).x1 == Catch::Approx(10));
    CHECK_FALSE(collide(shape, probe));
}

TEST_CASE("Rotation and resizing combine")
{
    auto shape = makeShape({ { 10, 0, 10, 0 } }, 100, 200);
    shape->resize(3);
    shape->rotate(-90);
    // (10, 0) * 3 = (30, 0); rotated anticlockwise -> (0, -30)
    const auto& line = shape->getVec().at(0);
    CHECK(line.x0 == Catch::Approx(0).margin(1e-9));
    CHECK(line.y0 == Catch::Approx(-30));
    // A point there in world coordinates collides
    CHECK(collide(shape, makeShape({ { -1, -30, 1, -30 } }, 100, 200)));
}

TEST_CASE("Contact with a line just touching points away from it, towards the shape's centre")
{
    // A wall along y = 10, from x = -50 to 50
    auto wall = makeShape({ { -50, 10, 50, 10 } });

    // A box centred on (0, 0) whose bottom edge is on the wall
    auto above = makeShape(
        { { -5, -10, 5, -10 }, { 5, -10, 5, 10 }, { 5, 10, -5, 10 }, { -5, 10, -5, -10 } });
    auto contact = above->contact(wall);
    REQUIRE(contact.has_value());
    CHECK(contact->normal.x() == Catch::Approx(0.0).margin(1e-9));
    CHECK(contact->normal.y() == Catch::Approx(-1.0));

    // The same box on the other side of the wall gets the opposite normal
    auto below = makeShape(
        { { -5, -10, 5, -10 }, { 5, -10, 5, 10 }, { 5, 10, -5, 10 }, { -5, 10, -5, -10 } }, 0, 20);
    contact = below->contact(wall);
    REQUIRE(contact.has_value());
    CHECK(contact->normal.y() == Catch::Approx(1.0));
}

TEST_CASE("Contact with a line cutting across a corner of a shape is along the line's normal")
{
    // A box centred on (0, 0), and a diagonal wall which cuts across its
    // bottom right corner, crossing its edges at (8, 10) and (10, 8)
    auto box = makeShape(
        { { -10, -10, 10, -10 }, { 10, -10, 10, 10 }, { 10, 10, -10, 10 }, { -10, 10, -10, -10 } });
    auto wall = makeShape({ { 3, 15, 15, 3 } });
    auto contact = box->contact(wall);
    REQUIRE(contact.has_value());
    // Halfway between where the wall crosses the box
    CHECK(contact->point.x() == Catch::Approx(9.0));
    CHECK(contact->point.y() == Catch::Approx(9.0));
    CHECK(contact->normal.x() == Catch::Approx(-std::sqrt(0.5)));
    CHECK(contact->normal.y() == Catch::Approx(-std::sqrt(0.5)));
}

TEST_CASE("Contact with the end of a line poking into a shape is along the shape's edge's normal")
{
    // A box centred on (0, 0), and a wall coming up from below whose end pokes
    // into it near its bottom right corner (like a wing hitting a corner)
    auto box = makeShape(
        { { -10, -10, 10, -10 }, { 10, -10, 10, 10 }, { 10, 10, -10, 10 }, { -10, 10, -10, -10 } });
    auto wall = makeShape({ { 8, 50, 8, 9 } });
    auto contact = box->contact(wall);
    REQUIRE(contact.has_value());
    // At the end of the wall
    CHECK(contact->point.x() == Catch::Approx(8.0));
    CHECK(contact->point.y() == Catch::Approx(9.0));
    // Pushed up, away from the wall, along the normal of the box's bottom edge
    // (rather than towards the box's centre)
    CHECK(contact->normal.x() == Catch::Approx(0.0).margin(1e-9));
    CHECK(contact->normal.y() == Catch::Approx(-1.0));
}

TEST_CASE("No contact when not touching")
{
    auto wall = makeShape({ { -50, 10, 50, 10 } });
    auto box
        = makeShape({ { -5, -5, 5, -5 }, { 5, -5, 5, 5 }, { 5, 5, -5, 5 }, { -5, 5, -5, -5 } });
    CHECK_FALSE(box->contact(wall).has_value());
}
