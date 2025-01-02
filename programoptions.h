#include <algorithm>
#include <string>
#include <vector>

namespace marengo{
namespace amaze{

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
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->m_tokens.begin(), this->m_tokens.end(), option);
        if (itr != this->m_tokens.end() && ++itr != this->m_tokens.end()) {
            return *itr;
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
