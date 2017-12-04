#include "helperfunctions.h"
#include "igraphicsadapter.h"

#include "boost/math/constants/constants.hpp"

#include <string>
#include <thread>
#include <mutex>

namespace
{

/* // TODO: remove?
int ClipFindRegion(
    const marengo::amaze::IGraphicsAdapter& gm,
    const int x,
    const int y
    )
{
    int rc = 0;
    if ( y >= gm.GetScreenHeight() )
    {
        rc |= 1; // top
    }
    else if ( y < 0 )
    {
        rc |= 2; // bottom
    }
    if ( x >= gm.GetScreenWidth() )
    {
        rc |= 4; // right
    }
    else if ( x < 0 )
    {
        rc |= 8; // left
    }
    return ( rc );
}*/
}

namespace marengo
{
namespace amaze
{
namespace helperfunctions
{

void CsvSplit(
    const std::string& s, char c, std::vector<std::string>& v )
{
    size_t j = s.find( c );
    if ( j == std::string::npos )
    {
        // no separators found
        v.push_back( s );
    }
    else
    {
        size_t i = 0;
        while ( j != std::string::npos )
        {
            v.push_back( s.substr( i, j - i ) );
            i = ++j;
            j = s.find( c, j );
            if ( j == std::string::npos )
            {
                v.push_back( s.substr( i, s.length() ) );
            }
        }
    }
}

void CsvSplit(
    const std::string& s, char c, std::vector<double>& v )
{
    std::vector<std::string> items;
    CsvSplit( s, c, items );
    for ( const std::string& item : items )
    {
        v.push_back( std::stod( item ) );
    }
}

double Cosine( int degrees )
{
    namespace bmc = boost::math::constants;
    // fast, cached lookup table implementation
    // for integer degrees. Thread safe initialisation.
    static std::once_flag initialised;
    degrees = std::abs( degrees % 360 );
    static double cosines[ 360 ];
    std::call_once( initialised, [&]() {
        for ( size_t n = 0; n < 360; ++n )
        {
            cosines[ n ] = std::cos( n / 360.0 ) * bmc::two_pi<double>();
        }
    } );
    return cosines[ degrees ];
}

double Sine( int degrees )
{
    namespace bmc = boost::math::constants;
    // fast, cached lookup table implementation
    // for integer degrees. Thread safe initialisation.
    static std::once_flag initialised;
    degrees = std::abs( degrees % 360 );
    static double sins[ 360 ];
    std::call_once( initialised, [&]() {
        for ( int n = 0; n < 360; ++n )
        {
            sins[ n ] = std::sin( n / 360.0 ) * bmc::two_pi<double>();
        }
    } );
    return sins[ degrees ];
}

int Sgn( int x )
{
    if ( x > 0 )
    {
        return 1;
    }
    if ( x < 0 )
    {
        return -1;
    }
    return 0;
}

bool DoLinesIntersect(
    long x1, long y1, long x2, long y2, long x3, long y3, long x4, long y4 )
{

    long Ax, Bx, Cx, Ay, By, Cy, d, e, f;
    short x1lo, x1hi, y1lo, y1hi;

    Ax = x2 - x1;
    Bx = x3 - x4;

    if ( Ax < 0 )
    {
        x1lo = (short)x2;
        x1hi = (short)x1;
    }
    else
    {
        x1hi = (short)x2;
        x1lo = (short)x1;
    }
    if ( Bx > 0 )
    {
        if ( x1hi < (short)x4 || (short)x3 < x1lo )
        {
            return false;
        }
    }
    else
    {
        if ( x1hi < (short)x3 || (short)x4 < x1lo )
        {
            return false;
        }
    }

    Ay = y2 - y1;
    By = y3 - y4;

    if ( Ay < 0 )
    {
        y1lo = (short)y2;
        y1hi = (short)y1;
    }
    else
    {
        y1hi = (short)y2;
        y1lo = (short)y1;
    }
    if ( By > 0 )
    {
        if ( y1hi < (short)y4 || (short)y3 < y1lo )
        {
            return false;
        }
    }
    else
    {
        if ( y1hi < (short)y3 || (short)y4 < y1lo )
        {
            return false;
        }
    }

    Cx = x1 - x3;
    Cy = y1 - y3;
    d = By * Cx - Bx * Cy; // alpha numerator
    f = Ay * Bx - Ax * By; // both denominator
    // alpha tests
    if ( f > 0 )
    {
        if ( d < 0 || d > f )
        {
            return false;
        }
    }
    else
    {
        if ( d > 0 || d < f )
        {
            return false;
        }
    }

    e = Ax * Cy - Ay * Cx; // beta numerator
    // beta tests
    if ( f > 0 )
    {
        if ( e < 0 || e > f )
        {
            return false;
        }
    }
    else
    {
        if ( e > 0 || e < f )
        {
            return false;
        }
    }

    // if we get here, the lines either intersect or are collinear.
    return true;
}

} // namespace helperfunctions
} // namespace amaze
} // namespace marengo
