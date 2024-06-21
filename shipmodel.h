#pragma once

#include <vector>
#include <memory>

#include "gameshape.h"
#include "igraphicsadapter.h"

namespace marengo
{
namespace amaze
{

class ShipModel
{
public:
    ShipModel( std::shared_ptr<GameShape> ship,
        std::shared_ptr<GameShape> flames,
        std::shared_ptr<GameShape> explosion );

    // Reset the ship model to a state ready for a new Level:
    void initialise();

    double fuel() const;
    void setFuel( double value );

    double rotation() const;
    void setRotation( double rotationDelta );

    double rotationDelta() const;
    void setRotationDelta( double value );

    bool slowRotation() const;
    void setSlowRotation( bool value );

    double dX() const;
    void setDx( double value );

    double dY() const;
    void setDy( double value );

    double velocity() const;
    void setVelocity( double value );

    double maxVelocity() const;
    void setMaxVelocity( double value );

    bool isExploding() const;
    void setIsExploding( bool value );

    bool isAccelerating() const;
    void setIsAccelerating( bool value );
    bool isBraking() const;
    void setIsBraking( bool value );

    const double maxFuel{300.0};

    double x() const;
    void setShipX( double value );

    double y() const;
    void setShipY( double value );

    void updateShipPosition();

    void setVisible( bool value );

    std::shared_ptr<GameShape> shipGameShape() const;
    std::shared_ptr<GameShape> flamesGameShape() const;
    std::shared_ptr<GameShape> explosionGameShape() const;

    void process();
    void drawFlames();
    void buildExplosionShape();

private:
    double m_Fuel;
    double m_Rotation{0.0};
    double m_RotationDelta{0.0};
    double m_ShipX;
    double m_ShipY;
    double m_Dx;
    double m_Dy;
    double m_Velocity{0};
    double m_MaxVelocity;
    bool m_IsExploding{false};
    bool m_IsAccelerating{false};
    bool m_IsBraking{false};

    std::shared_ptr<GameShape> m_ShipGameShape;
    std::shared_ptr<GameShape> m_FlamesGameShape;
    std::shared_ptr<GameShape> m_ExplosionGameShape;
};

} // namespace amaze
} // namespace marengo
