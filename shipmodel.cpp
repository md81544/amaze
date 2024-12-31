#include "shipmodel.h"
#include "gamemodel.h"
#include "helperfunctions.h"

#include <cmath>

namespace marengo {
namespace amaze {

ShipModel::ShipModel(
    std::shared_ptr<GameShape> ship,
    std::shared_ptr<GameShape> flames,
    std::shared_ptr<GameShape> explosion)
    : m_shipGameShape(ship)
    , m_flamesGameShape(flames)
    , m_explosionGameShape(explosion)
{
    m_shipGameShape->setColour(192, 192, 255, 255);
    m_shipGameShape->addLine(0, -20, 6, -11, 3);
    m_shipGameShape->addLine(6, -11, 6, 1, 3);
    m_shipGameShape->addLine(6, 1, 12, 7, 3);
    m_shipGameShape->addLine(12, 7, 12, 19, 3);
    m_shipGameShape->addLine(12, 19, 6, 13, 3);
    m_shipGameShape->addLine(6, 13, 2, 13, 3);
    m_shipGameShape->addLine(2, 13, 6, 16, 3);
    m_shipGameShape->addLine(6, 16, -6, 16, 3);
    m_shipGameShape->addLine(-6, 16, -2, 13, 3);
    m_shipGameShape->addLine(-2, 13, -6, 13, 3);
    m_shipGameShape->addLine(-6, 13, -12, 19, 3);
    m_shipGameShape->addLine(-12, 19, -12, 7, 3);
    m_shipGameShape->addLine(-12, 7, -6, 1, 3);
    m_shipGameShape->addLine(-6, 1, -6, -11, 3);
    m_shipGameShape->addLine(-6, -11, 0, -20, 3);
    m_shipGameShape->setName("Ship");
    m_shipGameShape->setVisible(true);

    m_flamesGameShape->setVisible(false);
    m_flamesGameShape->setGameShapeType(GameShapeType::NEUTRAL);
    m_flamesGameShape->setName("Flames");

    buildExplosionShape();
}

void ShipModel::initialise()
{
    // Resets the ship model to a state ready for a new Level
    m_rotation = 0.0;
    m_rotationDelta = 0.0;
    m_dx = 0.0;
    m_dy = 0.0;
    m_velocity = 0.0;
    m_maxVelocity = 0.0;
    m_isAccelerating = false;
}

double ShipModel::x() const
{
    return m_shipX;
}

void ShipModel::setShipX(double value)
{
    m_shipX = value;
}

double ShipModel::y() const
{
    return m_shipY;
}

void ShipModel::setShipY(double value)
{
    m_shipY = value;
}

double ShipModel::rotation() const
{
    return m_rotation;
}

void ShipModel::setRotation(double rotationDelta)
{
    m_rotation = std::fmod(m_rotation + rotationDelta, 360.0);
    // The ship object needs to be rotated the other way to
    // ensure it stays upright while the world rotates around it
    m_shipGameShape->rotate(-rotationDelta);
}

double ShipModel::rotationDelta() const
{
    return m_rotationDelta;
}

void ShipModel::setRotationDelta(double value)
{
    m_rotationDelta = value;
}

double ShipModel::dX() const
{
    return m_dx;
}

void ShipModel::setDx(double value)
{
    m_dx = value;
}

double ShipModel::dY() const
{
    return m_dy;
}

void ShipModel::setDy(double value)
{
    m_dy = value;
}

double ShipModel::velocity() const
{
    return m_velocity;
}

void ShipModel::setVelocity(double value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > m_maxVelocity) {
        value = m_maxVelocity;
    }
    m_velocity = value;
}

double ShipModel::maxVelocity() const
{
    return m_maxVelocity;
}

void ShipModel::setMaxVelocity(double value)
{
    m_maxVelocity = value;
}

void ShipModel::setIsExploding(bool value)
{
    m_explosionGameShape->setVisible(value);
    m_velocity = 0.0;
    m_dx = 0.0;
    m_dy = 0.0;
}

bool ShipModel::isAccelerating() const
{
    return m_isAccelerating;
}

void ShipModel::setIsAccelerating(bool value, float amount)
{
    m_isAccelerating = value;
    m_accelerationAmount = amount;
}

float ShipModel::accelerationAmount() const
{
    return m_accelerationAmount;
}

void ShipModel::updateShipPosition()
{
    // atmospheric friction:
    m_dx *= 0.9985;
    m_dy *= 0.9985;

    m_velocity = sqrt((m_dx * m_dx) + (m_dy * m_dy));

    if (m_isAccelerating) {
        m_dx = m_dx + helperfunctions::sine(m_rotation) * m_accelerationAmount;
        m_dy = m_dy + helperfunctions::cosine(m_rotation) * m_accelerationAmount;
    }

    // calculate new position for ship
    m_shipX = m_shipX - m_dx;
    m_shipY = m_shipY - m_dy;

    m_shipGameShape->setPos(m_shipX, m_shipY);
}

void ShipModel::setVisible(bool value)
{
    m_shipGameShape->setVisible(value);
}

std::shared_ptr<GameShape> ShipModel::shipGameShape() const
{
    return m_shipGameShape;
}

std::shared_ptr<GameShape> ShipModel::flamesGameShape() const
{
    return m_flamesGameShape;
}

void ShipModel::process(bool isExploding)
{
    // process() is called each game loop by the controlller
    setRotation(m_rotationDelta);

    updateShipPosition();

    if (isExploding) {
        m_explosionGameShape->setPos(m_shipX, m_shipY);
        m_explosionGameShape->resize(1.2);
        m_flamesGameShape->setVisible(false);
        m_isAccelerating = false;
    }

    if (m_isAccelerating) {
        m_flamesGameShape->setVisible(true);
        drawFlames();
    } else {
        m_flamesGameShape->setVisible(false);
    }
}

void ShipModel::drawFlames()
{
    // m_AccelerationAmount will be something like a maximum of 0.025
    float lengthMultiplier = m_accelerationAmount * 40.f;
    if (lengthMultiplier > 1.f) {
        lengthMultiplier = 1.f;
    }
    m_flamesGameShape->clear();
    for (int n = 0; n < 8; ++n) {
        m_flamesGameShape->setColour(180, 0, 0, 255);
        int bottomY = lengthMultiplier * 80 + rand() % 20;
        if (bottomY < 17) {
            bottomY = 17 + rand() % 20;
        }
        m_flamesGameShape->addLine(4 - rand() % 8, 17, 10 - rand() % 20, bottomY, 3);
    }
    for (int n = 0; n < 10; ++n) {
        m_flamesGameShape->setColour(255, 0, 0, 255);
        int bottomY = lengthMultiplier * 40 + rand() % 20;
        if (bottomY < 17) {
            bottomY = 17 + rand() % 20;
        }
        m_flamesGameShape->addLine(4 - rand() % 8, 17, 10 - rand() % 20, bottomY, 3);
    }
    for (int n = 0; n < 6; ++n) {
        m_flamesGameShape->setColour(250, 214, 116, 255);
        int bottomY = lengthMultiplier * 35 + rand() % 20;
        if (bottomY < 17) {
            bottomY = 17 + rand() % 20;
        }
        m_flamesGameShape->addLine(4 - rand() % 8, 17, 10 - rand() % 20, bottomY, 3);
    }
    for (int n = 0; n < 6; ++n) {
        m_flamesGameShape->setColour(255, 255, 255, 255);
        int bottomY = lengthMultiplier * 20 + rand() % 20;
        if (bottomY < 17) {
            bottomY = 17 + rand() % 20;
        }
        m_flamesGameShape->addLine(4 - rand() % 8, 17, 4 - rand() % 8, bottomY, 3);
    }

    // Ensure that the flames have the same rotation
    // and position as the ship:
    m_flamesGameShape->rotate(std::fmod(360 - m_rotation, 360));
    m_flamesGameShape->setPos(m_shipX, m_shipY);
}

void ShipModel::buildExplosionShape()
{
    using namespace helperfunctions;

    m_explosionGameShape->clear();
    // debris:
    for (int n = 0; n < 11; ++n) {
        if (rand() % 3 == 1) {
            continue;
        }
        double x1 = static_cast<double>(sine(n * 30.0) * 20 + (rand() % 10));
        double y1 = static_cast<double>(cosine(n * 30.0) * 20 + (rand() % 10));
        double x2 = static_cast<double>(sine((n + 1) * 30.0) * 20 + (rand() % 10));
        double y2 = static_cast<double>(cosine((n + 1) * 30.0) * 20 + (rand() % 10));
        uint8_t r = (rand() % 128) + 96;
        m_explosionGameShape->addShapeLine(
            ShapeLine { x1, y1, x2, y2, r, r, r, 255, 4, x1, y1, x2, y2 });
    }
    // flames
    for (int n = 0; n < 60; ++n) {
        if (rand() % 3 == 1) {
            continue;
        }
        double x1 = static_cast<double>(sine(n * 6.0) * 40);
        double y1 = static_cast<double>(cosine(n * 6.0) * 40);
        double x2 = static_cast<double>(sine(n * 6.0) * 50);
        double y2 = static_cast<double>(cosine(n * 6.0) * 50);
        m_explosionGameShape->addShapeLine(
            ShapeLine { x1, y1, x2, y2, 255, 255, 255, 255, 3, x1, y1, x2, y2 });
        x1 = static_cast<double>(sine(n * 6.0) * 50);
        y1 = static_cast<double>(cosine(n * 6.0) * 50);
        x2 = static_cast<double>(sine(n * 6.0) * 60);
        y2 = static_cast<double>(cosine(n * 6.0) * 60);
        m_explosionGameShape->addShapeLine(
            ShapeLine { x1, y1, x2, y2, 250, 214, 116, 255, 2, x1, y1, x2, y2 });
        int i = rand() % 50;
        x1 = static_cast<double>(sine(n * 6.0) * 60);
        y1 = static_cast<double>(cosine(n * 6.0) * 60);
        x2 = static_cast<double>(sine(n * 6.0) * (70 + i));
        y2 = static_cast<double>(cosine(n * 6.0) * (70 + i));
        m_explosionGameShape->addShapeLine(
            ShapeLine { x1, y1, x2, y2, 255, 0, 0, 255, 1, x1, y1, x2, y2 });
    }
    m_explosionGameShape->setName("Explosion");
    m_explosionGameShape->setVisible(false);
    m_explosionGameShape->resize(0.1);
}

std::shared_ptr<GameShape> ShipModel::explosionGameShape() const
{
    return m_explosionGameShape;
}

} // namespace amaze
} // namespace marengo
