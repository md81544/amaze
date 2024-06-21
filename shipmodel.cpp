#include "gamemodel.h"
#include "helperfunctions.h"

#include <cmath>

namespace marengo
{
namespace amaze
{

ShipModel::ShipModel( std::shared_ptr<GameShape> ship,
    std::shared_ptr<GameShape> flames, std::shared_ptr<GameShape> explosion )
    : m_ShipGameShape( ship ), m_FlamesGameShape( flames ),
      m_ExplosionGameShape( explosion )
{
    m_ShipGameShape->SetColour( 192, 192, 255, 255 );
    m_ShipGameShape->AddLine( 0, -20, 6, -11 );
    m_ShipGameShape->AddLine( 6, -11, 6, 1 );
    m_ShipGameShape->AddLine( 6, 1, 12, 7 );
    m_ShipGameShape->AddLine( 12, 7, 12, 19 );
    m_ShipGameShape->AddLine( 12, 19, 6, 13 );
    m_ShipGameShape->AddLine( 6, 13, 2, 13 );
    m_ShipGameShape->AddLine( 2, 13, 6, 16 );
    m_ShipGameShape->AddLine( 6, 16, -6, 16 );
    m_ShipGameShape->AddLine( -6, 16, -2, 13 );
    m_ShipGameShape->AddLine( -2, 13, -6, 13 );
    m_ShipGameShape->AddLine( -6, 13, -12, 19 );
    m_ShipGameShape->AddLine( -12, 19, -12, 7 );
    m_ShipGameShape->AddLine( -12, 7, -6, 1 );
    m_ShipGameShape->AddLine( -6, 1, -6, -11 );
    m_ShipGameShape->AddLine( -6, -11, 0, -20 );
    m_ShipGameShape->SetName( "Ship" );
    m_ShipGameShape->SetVisible( true );

    m_FlamesGameShape->SetVisible( false );
    m_FlamesGameShape->SetGameShapeType( GameShapeType::NEUTRAL );
    m_FlamesGameShape->SetName( "Flames" );

    buildExplosionShape();
}

void ShipModel::initialise()
{
    // Resets the ship model to a state ready for a new Level
    m_Rotation = 0;
    m_RotationDelta = 0;
    m_SlowRotation = true;
    m_Dx = 0.0;
    m_Dy = 0.0;
    m_Velocity = 0.0;
    m_MaxVelocity = 0.0;
    m_IsExploding = false;
    m_IsAccelerating = false;
    m_IsBraking = false;
}

double ShipModel::fuel() const { return m_Fuel; }

void ShipModel::setFuel( double value )
{
    if ( value > maxFuel )
    {
        m_Fuel = maxFuel;
    }
    else
    {
        m_Fuel = value;
    }
}

double ShipModel::x() const { return m_ShipX; }

void ShipModel::setShipX( double value ) { m_ShipX = value; }

double ShipModel::y() const { return m_ShipY; }

void ShipModel::setShipY( double value ) { m_ShipY = value; }

double ShipModel::rotation() const { return m_Rotation; }

void ShipModel::setRotation( int rotationDelta )
{
    m_Rotation = std::fmod( m_Rotation + rotationDelta, 360.0 );
    // The ship object needs to be rotated the other way to
    // ensure it stays upright while the world rotates around it
    m_ShipGameShape->Rotate( -rotationDelta );
}

int ShipModel::rotationDelta() const {return m_RotationDelta; }

void ShipModel::setRotationDelta( int value ) { m_RotationDelta = value; }

bool ShipModel::slowRotation() const { return m_SlowRotation; }

void ShipModel::setSlowRotation( bool value ) { m_SlowRotation = value; }

double ShipModel::dX() const { return m_Dx; }

void ShipModel::setDx( double value ) { m_Dx = value; }

double ShipModel::dY() const { return m_Dy; }

void ShipModel::setDy( double value ) { m_Dy = value; }

double ShipModel::velocity() const { return m_Velocity; }

void ShipModel::setVelocity( double value )
{
    if ( value < 0 )
    {
        value = 0;
    }
    if ( value > m_MaxVelocity )
    {
        value = m_MaxVelocity;
    }
    m_Velocity = value;
}

double ShipModel::maxVelocity() const { return m_MaxVelocity; }

void ShipModel::setMaxVelocity( double value ) { m_MaxVelocity = value; }

bool ShipModel::isExploding() const { return m_IsExploding; }

void ShipModel::setIsExploding( bool value )
{
    m_ExplosionGameShape->SetVisible( value );
    m_IsExploding = value;
}

bool ShipModel::isAccelerating() const { return m_IsAccelerating; }

void ShipModel::setIsAccelerating( bool value ) { m_IsAccelerating = value; }

bool ShipModel::isBraking() const { return m_IsBraking; }

void ShipModel::setIsBraking( bool value ) { m_IsBraking = value; }

void ShipModel::updateShipPosition()
{
    // atmospheric friction:
    m_Dx *= 0.998;
    m_Dy *= 0.998;

    m_Velocity = sqrt( ( m_Dx * m_Dx ) + ( m_Dy * m_Dy ) );

    if ( m_IsBraking && m_Fuel > 0 )
    {
        m_Fuel = m_Fuel - m_Velocity;
        m_Dx *= 0.9;
        if ( abs( m_Dx ) < 0.01 )
        {
            m_Dx = 0;
        }
        m_Dy *= 0.9;
        if ( abs( m_Dy ) < 0.01 )
        {
            m_Dy = 0;
        }
    }

    if ( m_IsAccelerating )
    {
        m_Dx = m_Dx + helperfunctions::Sine( m_Rotation ) * 0.04;
        m_Dy = m_Dy + helperfunctions::Cosine( m_Rotation ) * 0.04;
    }

    // calculate new position for ship
    m_ShipX = m_ShipX - m_Dx;
    m_ShipY = m_ShipY - m_Dy;

    m_ShipGameShape->SetPos( m_ShipX, m_ShipY );
}

void ShipModel::setVisible( bool value )
{
    m_ShipGameShape->SetVisible( value );
}

std::shared_ptr<GameShape> ShipModel::shipGameShape() const
{
    return m_ShipGameShape;
}

std::shared_ptr<GameShape> ShipModel::flamesGameShape() const
{
    return m_FlamesGameShape;
}

void ShipModel::process()
{
    // process() is called each game loop by the controlller
    setRotation( m_RotationDelta );

    updateShipPosition();

    if ( m_IsExploding )
    {
        m_ExplosionGameShape->SetPos( m_ShipX, m_ShipY );
        m_ExplosionGameShape->SetScale( 1.2 );
        m_FlamesGameShape->SetVisible( false );
        m_IsAccelerating = false;
    }

    if ( m_IsAccelerating )
    {
        m_FlamesGameShape->SetVisible( true );
        drawFlames();
    }
    else
    {
        m_FlamesGameShape->SetVisible( false );
    }
}

void ShipModel::drawFlames()
{
    m_FlamesGameShape->Clear();
    for ( int n = 0; n < 8; ++n )
    {
        m_FlamesGameShape->SetColour( 180, 0, 0, 255 );
        m_FlamesGameShape->AddLine(
            4 - rand() % 8, 17, 10 - rand() % 20, 80 + rand() % 20 );
    }
    for ( int n = 0; n < 10; ++n )
    {
        m_FlamesGameShape->SetColour( 255, 0, 0, 255 );
        m_FlamesGameShape->AddLine(
            4 - rand() % 8, 17, 15 - rand() % 30, 40 + rand() % 30 );
    }
    for ( int n = 0; n < 6; ++n )
    {
        m_FlamesGameShape->SetColour( 250, 214, 116, 255 );
        m_FlamesGameShape->AddLine(
            4 - rand() % 8, 17, 10 - rand() % 20, 35 + rand() % 20 );
    }
    for ( int n = 0; n < 6; ++n )
    {
        m_FlamesGameShape->SetColour( 255, 255, 255, 255 );
        m_FlamesGameShape->AddLine(
            4 - rand() % 8, 17, 4 - rand() % 8, 20 + rand() % 10 );
    }

    // Ensure that the flames have the same rotation
    // and position as the ship:
    m_FlamesGameShape->Rotate( std::fmod( 360 - m_Rotation, 360 ));
    m_FlamesGameShape->SetPos( m_ShipX, m_ShipY );
}

void ShipModel::buildExplosionShape()
{
    using namespace helperfunctions;

    m_ExplosionGameShape->Clear();
    // debris:
    for ( int n = 0; n < 11; ++n )
    {
        if ( rand() % 3 == 1 )
        {
            continue;
        }
        double x1 =
            static_cast<double>( Sine( n * 30.0 ) * 20 + ( rand() % 10 ) );
        double y1 =
            static_cast<double>( Cosine( n * 30.0 ) * 20 + ( rand() % 10 ) );
        double x2 = static_cast<double>(
            Sine( ( n + 1 ) * 30.0 ) * 20 + ( rand() % 10 ) );
        double y2 = static_cast<double>(
            Cosine( ( n + 1 ) * 30.0 ) * 20 + ( rand() % 10 ) );
        uint8_t r = ( rand() % 128 ) + 96;
        m_ExplosionGameShape->AddShapeLine(
            ShapeLine{ x1, y1, x2, y2, r, r, r, 255, 4 } );
    }
    // flames
    for ( int n = 0; n < 60; ++n )
    {
        if ( rand() % 3 == 1 )
        {
            continue;
        }
        double x1 = static_cast<double>( Sine( n * 6.0 ) * 40 );
        double y1 = static_cast<double>( Cosine( n * 6.0 ) * 40 );
        double x2 = static_cast<double>( Sine( n * 6.0 ) * 50 );
        double y2 = static_cast<double>( Cosine( n * 6.0 ) * 50 );
        m_ExplosionGameShape->AddShapeLine(
            ShapeLine{ x1, y1, x2, y2, 255, 255, 255, 255, 3 } );
        x1 = static_cast<double>( Sine( n * 6.0 ) * 50 );
        y1 = static_cast<double>( Cosine( n * 6.0 ) * 50 );
        x2 = static_cast<double>( Sine( n * 6.0 ) * 60 );
        y2 = static_cast<double>( Cosine( n * 6.0 ) * 60 );
        m_ExplosionGameShape->AddShapeLine(
            ShapeLine{ x1, y1, x2, y2, 250, 214, 116, 255, 2 } );
        int i = rand() % 50;
        x1 = static_cast<double>( Sine( n * 6.0 ) * 60 );
        y1 = static_cast<double>( Cosine( n * 6.0 ) * 60 );
        x2 = static_cast<double>( Sine( n * 6.0 ) * ( 70 + i ) );
        y2 = static_cast<double>( Cosine( n * 6.0 ) * ( 70 + i ) );
        m_ExplosionGameShape->AddShapeLine(
            ShapeLine{ x1, y1, x2, y2, 255, 0, 0, 255, 1 } );
    }
    m_ExplosionGameShape->SetName( "Explosion" );
    m_ExplosionGameShape->SetVisible( false );
    m_ExplosionGameShape->SetScale( 0.1 );
}

std::shared_ptr<GameShape> ShipModel::explosionGameShape() const
{
    return m_ExplosionGameShape;
}

} // namespace amaze
} // namespace marengo
