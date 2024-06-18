#pragma once

#include <map>
#include <string>

namespace marengo
{
namespace amaze
{

class Preferences
{
public:
    Preferences( const std::string& configFile );
    std::string GetString(
        const std::string& key, const std::string& defaultValue );
    long GetLong( const std::string& key, long defaultValue );
    void SetString( const std::string& key, const std::string& value );
    void SetLong( const std::string& key, long value );
    void Save();

private:
    std::string m_filename;
    std::map<std::string, std::string> keyValueMap;
};

} // namespace amaze
} // namespace marengo
