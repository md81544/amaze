#include "gamemodel.h"
#include "helperfunctions.h"

#include <cmath>

namespace marengo {
namespace amaze {

ShipModel::ShipModel(
    std::shared_ptr<GameShape> ship,
    std::shared_ptr<GameShape> flames,
    std::shared_ptr<GameShape> explosion)
    : m_ShipGameShape(ship)
    , m_FlamesGameShape(flames)
    , m_ExplosionGameShape(explosion)
{
    m_ShipGameShape->setColour(192, 192, 255, 255);
    m_ShipGameShape->addLine(0, -20, 6, -11, 3);
    m_ShipGameShape->addLine(6, -11, 6, 1, 3);
    m_ShipGameShape->addLine(6, 1, 12, 7, 3);
    m_ShipGameShape->addLine(12, 7, 12, 19, 3);
    m_ShipGameShape->addLine(12, 19, 6, 13, 3);
    m_ShipGameShape->addLine(6, 13, 2, 13, 3);
    m_ShipGameShape->addLine(2, 13, 6, 16, 3);
    m_ShipGameShape->addLine(6, 16, -6, 16, 3);
    m_ShipGameShape->addLine(-6, 16, -2, 13, 3);
    m_ShipGameShape->addLine(-2, 13, -6, 13, 3);
    m_ShipGameShape->addLine(-6, 13, -12, 19, 3);
    m_ShipGameShape->addLine(-12, 19, -12, 7, 3);
    m_ShipGameShape->addLine(-12, 7, -6, 1, 3);
    m_ShipGameShape->addLine(-6, 1, -6, -11, 3);
    m_ShipGameShape->addLine(-6, -11, 0, -20, 3);
    m_ShipGameShape->setName("Ship");
    m_ShipGameShape->setVisible(true);

    m_FlamesGameShape->setVisible(false);
    m_FlamesGameShape->setGameShapeType(GameShapeType::NEUTRAL);
    m_FlamesGameShape->setName("Flames");

    buildExplosionShape();
}

void ShipModel::initialise()
{
    // Resets the ship model to a state ready for a new Level
    m_Rotation = 0.0;
    m_RotationDelta = 0.0;
    m_Dx = 0.0;
    m_Dy = 0.0;
    m_Velocity = 0.0;
    m_MaxVelocity = 0.0;
    m_IsAccelerating = false;
}

double ShipModel::x() const
{
    return m_ShipX;
}

void ShipModel::setShipX(double value)
{
    m_ShipX = value;
}

double ShipModel::y() const
{
    return m_ShipY;
}

void ShipModel::setShipY(double value)
{
    m_ShipY = value;
}

double ShipModel::rotation() const
{
    return m_Rotation;
}

void ShipModel::setRotation(double rotationDelta)
{
    m_Rotation = std::fmod(m_Rotation + rotationDelta, 360.0);
    // The ship object needs to be rotated the other way to
    // ensure it stays upright while the world rotates around it
    m_ShipGameShape->rotate(-rotationDelta);
}

double ShipModel::rotationDelta() const
{
    return m_RotationDelta;
}

void ShipModel::setRotationDelta(double value)
{
    m_RotationDelta = value;
}

double ShipModel::dX() const
{
    return m_Dx;
}

void ShipModel::setDx(double value)
{
    m_Dx = value;
}

double ShipModel::dY() const
{
    return m_Dy;
}

void ShipModel::setDy(double value)
{
    m_Dy = value;
}

double ShipModel::velocity() const
{
    return m_Velocity;
}

void ShipModel::setVelocity(double value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > m_MaxVelocity) {
        value = m_MaxVelocity;
    }
    m_Velocity = value;
}

double ShipModel::maxVelocity() const
{
    return m_MaxVelocity;
}

void ShipModel::setMaxVelocity(double value)
{
    m_MaxVelocity = value;
}

void ShipModel::setIsExploding(bool value)
{
    m_ExplosionGameShape->setVisible(value);
    m_Velocity = 0.0;
    m_Dx = 0.0;
    m_Dy = 0.0;
}

bool ShipModel::isAccelerating() const
{
    return m_IsAccelerating;
}

void ShipModel::setIsAccelerating(bool value, float amount)
{
    m_IsAccelerating = value;
    m_AccelerationAmount = amount;
}

float ShipModel::accelerationAmount() const
{
    return m_AccelerationAmount;
}

void ShipModel::updateShipPosition()
{
    // atmospheric friction:
    m_Dx *= 0.9985;
    m_Dy *= 0.9985;

    m_Velocity = sqrt((m_Dx * m_Dx) + (m_Dy * m_Dy));

    if (m_IsAccelerating) {
        m_Dx = m_Dx + helperfunctions::Sine(m_Rotation) * m_AccelerationAmount;
        m_Dy = m_Dy + helperfunctions::Cosine(m_Rotation) * m_AccelerationAmount;
    }

    // calculate new position for ship
    m_ShipX = m_ShipX - m_Dx;
    m_ShipY = m_ShipY - m_Dy;

    m_ShipGameShape->setPos(m_ShipX, m_ShipY);
}

void ShipModel::setVisible(bool value)
{
    m_ShipGameShape->setVisible(value);
}

std::shared_ptr<GameShape> ShipModel::shipGameShape() const
{
    return m_ShipGameShape;
}

std::shared_ptr<GameShape> ShipModel::flamesGameShape() const
{
    return m_FlamesGameShape;
}

void ShipModel::process(bool isExploding)
{
    // process() is called each game loop by the controlller
    setRotation(m_RotationDelta);

    updateShipPosition();

    if (isExploding) {
        m_ExplosionGameShape->setPos(m_ShipX, m_ShipY);
        m_ExplosionGameShape->setScale(1.2);
        m_FlamesGameShape->setVisible(false);
        m_IsAccelerating = false;
    }

    if (m_IsAccelerating) {
        m_FlamesGameShape->setVisible(true);
        drawFlames();
    } else {
        m_FlamesGameShape->setVisible(false);
    }
}

void ShipModel::drawFlames()
{
    m_FlamesGameShape->clear();
    for (int n = 0; n < 8; ++n) {
        m_FlamesGameShape->setColour(180, 0, 0, 255);
        m_FlamesGameShape->addLine(4 - rand() % 8, 17, 10 - rand() % 20, 80 + rand() % 20, 3);
    }
    for (int n = 0; n < 10; ++n) {
        m_FlamesGameShape->setColour(255, 0, 0, 255);
        m_FlamesGameShape->addLine(4 - rand() % 8, 17, 15 - rand() % 30, 40 + rand() % 30, 3);
    }
    for (int n = 0; n < 6; ++n) {
        m_FlamesGameShape->setColour(250, 214, 116, 255);
        m_FlamesGameShape->addLine(4 - rand() % 8, 17, 10 - rand() % 20, 35 + rand() % 20, 3);
    }
    for (int n = 0; n < 6; ++n) {
        m_FlamesGameShape->setColour(255, 255, 255, 255);
        m_FlamesGameShape->addLine(4 - rand() % 8, 17, 4 - rand() % 8, 20 + rand() % 10, 3);
    }

    // Ensure that the flames have the same rotation
    // and position as the ship:
    m_FlamesGameShape->rotate(std::fmod(360 - m_Rotation, 360));
    m_FlamesGameShape->setPos(m_ShipX, m_ShipY);
}

void ShipModel::buildExplosionShape()
{
    using namespace helperfunctions;

    m_ExplosionGameShape->clear();
    // debris:
    for (int n = 0; n < 11; ++n) {
        if (rand() % 3 == 1) {
            continue;
        }
        double x1 = static_cast<double>(Sine(n * 30.0) * 20 + (rand() % 10));
        double y1 = static_cast<double>(Cosine(n * 30.0) * 20 + (rand() % 10));
        double x2 = static_cast<double>(Sine((n + 1) * 30.0) * 20 + (rand() % 10));
        double y2 = static_cast<double>(Cosine((n + 1) * 30.0) * 20 + (rand() % 10));
        uint8_t r = (rand() % 128) + 96;
        m_ExplosionGameShape->addShapeLine(ShapeLine { x1, y1, x2, y2, r, r, r, 255, 4 });
    }
    // flames
    for (int n = 0; n < 60; ++n) {
        if (rand() % 3 == 1) {
            continue;
        }
        double x1 = static_cast<double>(Sine(n * 6.0) * 40);
        double y1 = static_cast<double>(Cosine(n * 6.0) * 40);
        double x2 = static_cast<double>(Sine(n * 6.0) * 50);
        double y2 = static_cast<double>(Cosine(n * 6.0) * 50);
        m_ExplosionGameShape->addShapeLine(ShapeLine { x1, y1, x2, y2, 255, 255, 255, 255, 3 });
        x1 = static_cast<double>(Sine(n * 6.0) * 50);
        y1 = static_cast<double>(Cosine(n * 6.0) * 50);
        x2 = static_cast<double>(Sine(n * 6.0) * 60);
        y2 = static_cast<double>(Cosine(n * 6.0) * 60);
        m_ExplosionGameShape->addShapeLine(ShapeLine { x1, y1, x2, y2, 250, 214, 116, 255, 2 });
        int i = rand() % 50;
        x1 = static_cast<double>(Sine(n * 6.0) * 60);
        y1 = static_cast<double>(Cosine(n * 6.0) * 60);
        x2 = static_cast<double>(Sine(n * 6.0) * (70 + i));
        y2 = static_cast<double>(Cosine(n * 6.0) * (70 + i));
        m_ExplosionGameShape->addShapeLine(ShapeLine { x1, y1, x2, y2, 255, 0, 0, 255, 1 });
    }
    m_ExplosionGameShape->setName("Explosion");
    m_ExplosionGameShape->setVisible(false);
    m_ExplosionGameShape->setScale(0.1);
}

std::shared_ptr<GameShape> ShipModel::explosionGameShape() const
{
    return m_ExplosionGameShape;
}

} // namespace amaze
} // namespace marengo
