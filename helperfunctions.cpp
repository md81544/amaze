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

bool doLinesIntersect(long x1, long y1, long x2, long y2, long x3, long y3, long x4, long y4)
{

    long Ax, Bx, Cx, Ay, By, Cy, d, e, f;
    short x1lo, x1hi, y1lo, y1hi;

    Ax = x2 - x1;
    Bx = x3 - x4;

    if (Ax < 0) {
        x1lo = (short)x2;
        x1hi = (short)x1;
    } else {
        x1hi = (short)x2;
        x1lo = (short)x1;
    }
    if (Bx > 0) {
        if (x1hi < (short)x4 || (short)x3 < x1lo) {
            return false;
        }
    } else {
        if (x1hi < (short)x3 || (short)x4 < x1lo) {
            return false;
        }
    }

    Ay = y2 - y1;
    By = y3 - y4;

    if (Ay < 0) {
        y1lo = (short)y2;
        y1hi = (short)y1;
    } else {
        y1hi = (short)y2;
        y1lo = (short)y1;
    }
    if (By > 0) {
        if (y1hi < (short)y4 || (short)y3 < y1lo) {
            return false;
        }
    } else {
        if (y1hi < (short)y3 || (short)y4 < y1lo) {
            return false;
        }
    }

    Cx = x1 - x3;
    Cy = y1 - y3;
    d = By * Cx - Bx * Cy; // alpha numerator
    f = Ay * Bx - Ax * By; // both denominator
    // alpha tests
    if (f > 0) {
        if (d < 0 || d > f) {
            return false;
        }
    } else {
        if (d > 0 || d < f) {
            return false;
        }
    }

    e = Ax * Cy - Ay * Cx; // beta numerator
    // beta tests
    if (f > 0) {
        if (e < 0 || e > f) {
            return false;
        }
    } else {
        if (e > 0 || e < f) {
            return false;
        }
    }

    // if we get here, the lines either intersect or are collinear.
    return true;
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
