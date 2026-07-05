#include "levelfilelister.h"
#include "exceptions.h"
#include "utils.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <regex>
#include <vector>

namespace marengo {
namespace amaze {

namespace {
std::string getLevelDescription(std::filesystem::path levelFile)
{
    std::ifstream in(levelFile.string());
    if (!in) {
        THROWUP(AmazeRuntimeException, "Failed to load Level file " + levelFile.string());
    }
    std::string currentLine;
    std::string levelDescription { "Untitled" };
    getline(in, currentLine);
    std::vector<std::string> vec;
    utils::csvSplit(currentLine, '~', vec);
    if (vec.size() > 6) {
        levelDescription = vec[6];
    }
    in.close();
    return levelDescription;
}
} // anonymous

LevelFileLister::LevelFileLister(std::string_view dataDir)
{
    std::regex regexPattern("^level.*cfg$");
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
                std::format("{} : {}", pr.first, getLevelDescription(pr.second)),
                pr.second.string() };
    }
}
std::map<int, FileData> LevelFileLister::getFileMap()
{
    return m_fileDataMap;
}

} // namespace amaze
} // namespace marengo