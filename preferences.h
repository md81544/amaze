#pragma once

#include <map>

class Preferences
{
public:
    Preferences( const std::string& configFile );
    std::string GetString( const std::string& key, const std::string& default );
    long GetLong( const std::string& key, long default );
    void SetString( const std::string& key, const std::string& value );
    void SetLong( const std::string& key, long value );
    void Save();

private:
    std::string m_filename;
    std::map<std::string, std::string> keyValueMap;
};
