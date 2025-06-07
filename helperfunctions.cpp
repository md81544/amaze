#include "helperfunctions.h"
#include "igraphicsadapter.h"
#include "log.h"

#include "boost/math/constants/constants.hpp"

#include <mutex>
#include <random>
#include <string>
#include <thread>

namespace marengo {
namespace amaze {
namespace helperfunctions {

void csvSplit(const std::string& s, char c, std::vector<std::string>& v)
{
    size_t j = s.find(c);
    if (j == std::string::npos) {
        // no separators found
        v.push_back(s);
    } else {
        size_t i = 0;
        while (j != std::string::npos) {
            v.push_back(s.substr(i, j - i));
            i = ++j;
            j = s.find(c, j);
            if (j == std::string::npos) {
                v.push_back(s.substr(i, s.length()));
            }
        }
    }
}

void csvSplit(const std::string& s, char c, std::vector<double>& v)
{
    std::vector<std::string> items;
    csvSplit(s, c, items);
    for (const std::string& item : items) {
        v.push_back(std::stod(item));
    }
}

double cosine(double degrees)
{
    namespace bmc = boost::math::constants;
    return std::cos(degrees * (bmc::two_pi<double>() / 360.0));
}

double sine(double degrees)
{
    namespace bmc = boost::math::constants;
    return std::sin(degrees * (bmc::two_pi<double>() / 360.0));
}

uint8_t rnd(uint8_t max)
{
    if (max < 2) {
        return 0;
    }
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, max - 1);
    return distrib(gen);
}

} // namespace helperfunctions
} // namespace amaze
} // namespace marengo
