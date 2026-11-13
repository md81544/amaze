// Tests for the arena -> canvas mapping used by View

#include "utils.h"
#include "viewtransform.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using marengo::amaze::arenaToCanvas;
namespace utils = marengo::amaze::utils;

namespace {

// The per-point calculation View used before arenaToCanvas()
QPointF reference(double x, double y, double shipX, double shipY, double rotation, QSizeF canvas)
{
    double scale = std::sqrt((canvas.width() * canvas.height()) / (480.0 * 300.0));
    double rx = x - shipX;
    double ry = y - shipY;
    double c = utils::cosine(rotation);
    double s = utils::sine(rotation);
    return { (rx * c - ry * s) * scale + canvas.width() / 2.0,
             (rx * s + ry * c) * scale + canvas.height() / 2.0 };
}

} // namespace

TEST_CASE("The centre of the view maps to the centre of the canvas")
{
    QPointF p = arenaToCanvas(437, 704, 270, QSizeF(1920, 1200)).map(QPointF(437, 704));
    CHECK(p.x() == Catch::Approx(960));
    CHECK(p.y() == Catch::Approx(600));
}

TEST_CASE("A 1920x1200 canvas shows 480x300 arena units")
{
    auto t = arenaToCanvas(0, 0, 0, QSizeF(1920, 1200));
    CHECK(t.map(QPointF(240, 150)).x() == Catch::Approx(1920));
    CHECK(t.map(QPointF(240, 150)).y() == Catch::Approx(1200));
    CHECK(t.map(QPointF(-240, -150)).x() == Catch::Approx(0).margin(1e-9));
    CHECK(t.map(QPointF(-240, -150)).y() == Catch::Approx(0).margin(1e-9));
}

TEST_CASE("arenaToCanvas matches View's previous calculation")
{
    const QSizeF canvases[] = { { 1920, 1200 }, { 2400, 960 }, { 1000, 1500 } };
    const double rotations[] = { 0, 1.5, 45, 90, 225, 270, 359.25, -30 };
    const QPointF points[] = { { 437, 704 }, { 400, 650 }, { 0, 0 }, { 1999.5, 3.25 } };
    for (const auto& canvas : canvases) {
        for (double rotation : rotations) {
            auto t = arenaToCanvas(437.25, 704.5, rotation, canvas);
            for (const auto& p : points) {
                QPointF expected = reference(p.x(), p.y(), 437.25, 704.5, rotation, canvas);
                QPointF actual = t.map(p);
                CHECK(actual.x() == Catch::Approx(expected.x()).margin(1e-9));
                CHECK(actual.y() == Catch::Approx(expected.y()).margin(1e-9));
            }
        }
    }
}
