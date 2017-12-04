#include "catch.hpp"
#include "../vectorfont.h"

// See
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

TEST_CASE( "Testing vector fonts" )
{
    auto vecSpace = marengo::vectorfont::getCharacter( ' ' );
    REQUIRE( vecSpace.size() == 0 );
    auto vecA = vector_font::GetCharacter( 'A' );
    REQUIRE( vecA.size() == 16 );
}
