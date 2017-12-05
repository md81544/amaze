#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

void PressAKey( std::string msg = "Press ENTER: " )
{
    std::cout << msg;
    std::cin.ignore();
    std::endl( std::cout );
}

int main( int argc, char* argv[] )
{
    int result = Catch::Session().run( argc, argv );
    PressAKey();
    return result;
}
