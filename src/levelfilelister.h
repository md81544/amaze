#pragma once

#include <expected>
#include <map>
#include <string>
#include <string_view>

// Class to simply enumerate through all level files and make them
// available for e.g. the "load level" dialogue box

namespace marengo {
namespace amaze {

struct FileData {
    int number;
    std::string description;
    std::string filename;
};

class LevelFileLister {
    public:
        explicit LevelFileLister(std::string_view dataDir);
        ~LevelFileLister(){}
        std::map<int, FileData> getFileMap();
        std::expected<std::string,std::string> getFilename(int index);
    private:
    std::map<int, FileData> m_fileDataMap;
};

} // namespace amaze
} // namespace marengo