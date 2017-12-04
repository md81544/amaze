#include "preferences.h"
#include "helperfunctions.h"
#include "exceptions.h"

#include "boost/algorithm/string.hpp"

#include <fstream>
#include <sstream>

namespace marengo
{
namespace amaze
{

Preferences::Preferences( const std::string& configFile )
{
    m_filename = configFile;
    // Open the config file
    std::ifstream ifs( configFile.c_str(), std::ios::in );
    if ( !ifs )
    {
        // if it fails, no problem, we just don't
        // have any settings other than defaults yet.
        // i.e. nothing has been saved yet
        return;
    }
    std::string sLine;
    size_t n;
    while ( !ifs.eof() )
    {
        getline( ifs, sLine );
        boost::algorithm::trim( sLine );
        if ( sLine != "" && sLine.substr( 0, 1 ) != "#" )
        {
            n = sLine.find( "=" );
            if ( n != std::string::npos )
            {
                std::string sKey = sLine.substr( 0, n );
                boost::algorithm::trim( sKey );
                std::string sValue = sLine.substr( n + 1 );
                boost::algorithm::trim( sValue );
                keyValueMap[ sKey ] = sValue;
            }
        }
    }
    ifs.close();
}

void Preferences::Save()
{
    std::ofstream ofs( m_filename.c_str(), std::ios::out );
    if ( !ofs )
    {
        THROWUP( amaze::exceptions::AmazeRuntimeException,
            "Could not open the config file for saving" );
    }
    for ( const auto& pr : keyValueMap )
    {
        std::string sLine;
        sLine = pr.first + "=" + pr.second;
        ofs << sLine;
    }
    ofs.close();
}

std::string Preferences::GetString(
    const std::string& key, const std::string& defaultValue )
{
    auto it = keyValueMap.find( key );
    if ( it == keyValueMap.end() )
    {
        return defaultValue;
    }
    return ( *it ).second;
}

long Preferences::GetLong( const std::string& key, long defaultValue )
{
    auto it = keyValueMap.find( key );
    if ( it == keyValueMap.end() )
    {
        return defaultValue;
    }
    return stol( ( *it ).second );
}

void Preferences::SetString(
    const std::string& sKey,
    const std::string& value
    )
{
    keyValueMap[ sKey ] = value;
}

void Preferences::SetLong( const std::string& sKey, long value )
{
    keyValueMap[ sKey ] = std::to_string( value );
}

} // namespace amaze
} // namespace marengo
