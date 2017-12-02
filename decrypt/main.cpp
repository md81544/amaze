#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>

std::vector<unsigned char> ReadFile( const std::string& filename )
{
    std::ifstream in( filename, std::ios::in | std::ios::binary );
    if ( !in )
    {
        throw std::runtime_error(
            "Could not open file " + filename + " for reading" );
    }
    std::vector<unsigned char> vec;
    in.seekg( 0, std::ios_base::end );
    std::streampos fileSize = in.tellg();
    vec.resize( fileSize );

    in.seekg( 0, std::ios_base::beg );
    in.read( reinterpret_cast<char*>( &vec[ 0 ] ), fileSize );
    return vec;
}


int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        std::cout << "Usage: decrypt <filename> "
            << "(pipe the output to a new file)" << std::endl;
        return 1;
    }
    auto vec = ReadFile( argv[ 1 ] );
    for ( const auto& c : vec )
    {
        if ( c == '9' ) // Line delimiter
        {
            std::cout << '\n';
        }
        else
        {
            std::cout << static_cast<char>((c ^ 69));
        }
    }
}
