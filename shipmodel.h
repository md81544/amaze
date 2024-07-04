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

    bool slowRotation() const;
    void setSlowRotation(bool value);

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
    double m_Rotation { 0.0 };
    double m_RotationDelta { 0.0 };
    double m_ShipX;
    double m_ShipY;
    double m_Dx;
    double m_Dy;
    double m_Velocity { 0.0 };
    double m_MaxVelocity;
    bool m_IsAccelerating { false };
    float m_AccelerationAmount { 0.f };

    std::shared_ptr<GameShape> m_ShipGameShape;
    std::shared_ptr<GameShape> m_FlamesGameShape;
    std::shared_ptr<GameShape> m_ExplosionGameShape;
};

} // namespace amaze
} // namespace marengo
