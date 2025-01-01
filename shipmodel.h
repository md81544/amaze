#pragma once

#include <memory>
#include <vector>

#include "gameshape.h"
#include "igraphicsadapter.h"

namespace marengo {
namespace amaze {

class ShipModel {
public:
    ShipModel(
        std::shared_ptr<GameShape> ship,
        std::shared_ptr<GameShape> flames,
        std::shared_ptr<GameShape> explosion);

    // Reset the ship model to a state ready for a new Level:
    void initialise();

    double rotation() const;
    void setRotation(double rotationDelta);

    double rotationDelta() const;
    void setRotationDelta(double value);

    double dX() const;
    void setDx(double value);

    double dY() const;
    void setDy(double value);

    double velocity() const;
    void setVelocity(double value);

    double maxVelocity() const;
    void setMaxVelocity(double value);

    bool isExploding() const;
    void setIsExploding(bool value);

    bool isAccelerating() const;
    void setIsAccelerating(bool value, float amount = 0.f);
    float accelerationAmount() const;

    double x() const;
    void setShipX(double value);

    double y() const;
    void setShipY(double value);

    void updateShipPosition();

    void setVisible(bool value);

    std::shared_ptr<GameShape> shipGameShape() const;
    std::shared_ptr<GameShape> flamesGameShape() const;
    std::shared_ptr<GameShape> explosionGameShape() const;

    void process(bool isExploding);
    void drawFlames();
    void buildExplosionShape();

private:
    double m_rotation { 0.0 };
    double m_rotationDelta { 0.0 };
    double m_shipX;
    double m_shipY;
    double m_dx;
    double m_dy;
    double m_velocity { 0.0 };
    double m_maxVelocity;
    bool m_isAccelerating { false };
    float m_accelerationAmount { 0.f };

    std::shared_ptr<GameShape> m_shipGameShape;
    std::shared_ptr<GameShape> m_flamesGameShape;
    std::shared_ptr<GameShape> m_explosionGameShape;
};

} // namespace amaze
} // namespace marengo
