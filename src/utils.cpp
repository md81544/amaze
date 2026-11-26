#include "utils.h"

#include <cmath>
#include <numbers>
#include <random>
#include <string>

namespace marengo {
namespace amaze {
namespace utils {

double cosine(double degrees)
{
    return std::cos(degrees * (std::numbers::pi / 180.0));
}

double sine(double degrees)
{
    return std::sin(degrees * (std::numbers::pi / 180.0));
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
