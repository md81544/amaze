#include "catch.hpp"
#include "../vector_font.h"

// See
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

TEST_CASE( "Testing vector fonts" )
{
    auto vecSpace = vector_font::GetCharacter( ' ' );
    REQUIRE( vecSpace.size() == 0 );
    auto vecA = vector_font::GetCharacter( 'A' );
    REQUIRE( vecA.size() == 16 );
}