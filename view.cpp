#include "view.h"
#include "shapeline.h"
#include "helperfunctions.h"

#include <vector>

namespace marengo
{
namespace amaze
{

View::View(
    GameModel& model,
    IGraphicsAdapter& gm
    )
    :
    m_Model( model ),
    m_GraphicsAdapter( gm )
{
}

void View::PlaySounds()
{
    if ( m_Model.ShipModel()->isAccelerating() )
    {
        m_GraphicsAdapter.SoundLoop( "rocket" );
    }
    else
    {
        m_GraphicsAdapter.SoundFade( "rocket", 1000 );
    }

    if ( m_Model.ShipModel()->isExploding() )
    {
        m_GraphicsAdapter.SoundPlay( "collision" );
    }
}

void View::Update()
{
    // TODO: this should take a model as its parameter; we shouldn't be bound
    // to one model but should be able to accept any model implementing a
    // specific interface. This would enable us to use models for splash
    // screens / high score screens etc.

    // This is where all state is "realised" i.e. images drawn, sounds played
    // etc.
    PlaySounds();

    // Dynamic shapes (i.e. shapes which rotate around the ship)
    m_Model.ProcessDynamicObjects( [&]( GameShape& shape )
    {
        if ( shape.IsVisible() && shape.IsActive() )
        {
            RotateAndDrawShape( shape );
        }
    } );

    // Static shapes (i.e. items which don't move on screen, e.g. the gauges)
    m_Model.ProcessStaticObjects( [&]( GameShape& shape )
    {
        if ( shape.IsVisible() )
        {
            DrawStaticShape( const_cast<const GameShape&>(shape) );
        }
    } );
}

void View::RotateAndDrawShape( const GameShape& shape ) const
{
    // We treat the viewport as representing 320 coordinates wide,
    // regardless of its physical dimensions:
    double scale = m_GraphicsAdapter.GetScreenWidth() / 320.0;
    double xOffset = m_GraphicsAdapter.GetScreenWidth() / 2;
    double yOffset = m_GraphicsAdapter.GetScreenHeight() / 2;

    for ( unsigned int line = 0; line < shape.GetVec().size(); ++line )
    {
        ShapeLine sl = shape.GetVec()[ line ];
        double x0 = sl.x0 + shape.GetPosX() - m_Model.ShipModel()->x();
        double y0 = sl.y0 + shape.GetPosY() - m_Model.ShipModel()->y();
        double x1 = sl.x1 + shape.GetPosX() - m_Model.ShipModel()->x();
        double y1 = sl.y1 + shape.GetPosY() - m_Model.ShipModel()->y();
        // OK, when we get here, we have a line expressed
        // relative to the origin of the ship.  We can apply the
        // rotate now (unless the shape is marked "don't rotate")...
        double dCos =
            HelperFunctions::Cosine( m_Model.ShipModel()->rotation() );
        double dSin = HelperFunctions::Sine( m_Model.ShipModel()->rotation() );
        double x0r, y0r, x1r, y1r;
        x0r = x0 * dCos - y0 * dSin;
        y0r = x0 * dSin + y0 * dCos;
        x1r = x1 * dCos - y1 * dSin;
        y1r = x1 * dSin + y1 * dCos;
        // now draw adjusted for physical screen coords
        // TODO use IGraphicsAdapter's drawLine function here
    }
}

void View::DrawStaticShape( const GameShape& shape ) const
{
    // Note that static images' coordinates' origin is TOP LEFT OF THE SCREEN
    double scale = m_GraphicsAdapter.GetScreenWidth() / 320.0;

    for ( const auto& sl : shape.GetVec() )
    {
        // TODO use IGraphicsAdapter's drawLine function here
    }
}

} // namespace amaze
} // namespace marengo
