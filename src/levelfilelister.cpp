#include "levelfilelister.h"
#include "exceptions.h"
#include "levelfile.h"
#include "log.h"

#include <expected>
#include <filesystem>
#include <format>
#include <map>
#include <regex>

namespace marengo {
namespace amaze {

namespace {
std::string getLevelDescription(const std::filesystem::path& levelFile)
{
    // Read the whole level, so this works whatever the file's format
    try {
        const std::string description = readLevelFile(levelFile.string()).description;
        return description.empty() ? "Untitled" : description;
    } catch (const AmazeRuntimeException& e) {
        mgo::Log::warn(
            std::format("Can't read the description of {}: {}", levelFile.string(), e.what()));
        return "Untitled";
    }
}
} // anonymous

LevelFileLister::LevelFileLister(std::string_view dataDir)
{
    std::regex regexPattern("^level.*\\.lvl$");
    std::map<int, std::filesystem::path> sortedPaths;
    for (const auto& entry : std::filesystem::directory_iterator(dataDir)) {
        if (entry.is_regular_file()) {
            const std::string fileName = entry.path().filename().string();
            if (std::regex_match(fileName, regexPattern)) {
                std::string num;
                for (const auto c : fileName) {
                    if (isdigit(c)) {
                        num += c;
                    }
                }
                sortedPaths[std::stoi(num)] = entry;
            }
        }
    }
    for (const auto& pr : sortedPaths) {
        m_fileDataMap[pr.first]
            = { pr.first,
                getLevelDescription(pr.second),
                pr.second.string() };
    }
}
std::map<int, FileData> LevelFileLister::getFileMap()
{
    return m_fileDataMap;
}

std::expected<std::string, std::string> LevelFileLister::getFilename(int index)
{
    auto it = m_fileDataMap.find(index);
    if (it != m_fileDataMap.end()) {
        return it->second.filename;
    }
    return std::unexpected { "Could not locate level at index" };
}

} // namespace amaze
} // namespace marengo