#include "catch.hpp"
#include "../vectorfont.h"

TEST_CASE( "Testing vector fonts" )
{
    using namespace marengo::amaze;
    auto vecSpace = vectorfont::getCharacter( ' ' );
    REQUIRE( vecSpace.size() == 0 );
    auto vecA = vectorfont::getCharacter( 'A' );
    REQUIRE( vecA.size() == 16 );
}
