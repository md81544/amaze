#include <algorithm>
#include <string>
#include <vector>

namespace marengo {
namespace amaze {

class ProgramOptions {
public:
    ProgramOptions(int& argc, char** argv)
    {
        for (int i = 1; i < argc; ++i) {
            this->m_tokens.push_back(std::string(argv[i]));
        }
    }
    const std::string& getCmdOption(const std::string& option) const
    {
        auto it = std::find(this->m_tokens.begin(), this->m_tokens.end(), option);
        if (it != this->m_tokens.end() && ++it != this->m_tokens.end()) {
            return *it;
        }
        static const std::string empty_string("");
        return empty_string;
    }
    bool cmdOptionExists(const std::string& option) const
    {
        return std::find(this->m_tokens.begin(), this->m_tokens.end(), option)
            != this->m_tokens.end();
    }

private:
    std::vector<std::string> m_tokens;
};

} // namespace amaze
} // namespace marengo
