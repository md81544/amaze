#pragma once

// Free function which will return a vector of ints - each pair of ints in the
// sequence represents an x,y vertex (y inverted for screen coordinates, i.e. 0
// is at the top). Note that (-1,-1) represents a "pen up", otherwise lines
// should be drawn from the previous point.
//
// Usage example:
//    std::vector<int> v = vectorfont::getCharacter('Z');

#include <vector>

namespace {
// The following vector describes each printable ASCII character as lines.
// The characters range from space (32) to tilde (126), that is, 95
// characters. The numbers are vertex pairs. A vertex of (-1,-1) indicates
// a "pen up" operation.
//
// Note this has been MODIFIED from the standard public-domain Hershey
// vector font, primarily by avoiding negative values for descenders and
// then inverting the Y values to fit with regular screen coordinates. The
// minimum vertex (apart from the "pen up") is 0,0 and the max X is 26. Max
// Y=33.
std::vector<std::vector<int>>
    characterLines = std::vector<std::vector<int>> {
        {},
        { 5, 5, 5, 19, -1, -1, 5, 24, 4, 25, 5, 26, 6, 25, 5, 24 },
        { 4, 5, 4, 12, -1, -1, 12, 5, 12, 12 },
        { 11, 1, 4, 33, -1, -1, 17, 1, 10, 33, -1, -1, 4, 14, 18, 14, -1, -1, 3, 20, 17, 20 },
        { 8,  1,  8,  30, -1, -1, 12, 1,  12, 30, -1, -1, 17, 8,  15, 6,  12, 5,
          8,  5,  5,  6,  3,  8,  3,  10, 4,  12, 5,  13, 7,  14, 13, 16, 15, 17,
          16, 18, 17, 20, 17, 23, 15, 25, 12, 26, 8,  26, 5,  25, 3,  23 },
        { 21, 5,  3,  26, -1, -1, 8,  5,  10, 7,  10, 9,  9,  11, 7,  12, 5,  12, 3,  10, 3,
          8,  4,  6,  6,  5,  8,  5,  10, 6,  13, 7,  16, 7,  19, 6,  21, 5,  -1, -1, 17, 19,
          15, 20, 14, 22, 14, 24, 16, 26, 18, 26, 20, 25, 21, 23, 21, 21, 19, 19, 17, 19 },
        { 23, 14, 23, 13, 22, 12, 21, 12, 20, 13, 19, 15, 17, 20, 15, 23, 13, 25, 11, 26, 7,  26, 5,
          25, 4,  24, 3,  22, 3,  20, 4,  18, 5,  17, 12, 13, 13, 12, 14, 10, 14, 8,  13, 6,  11, 5,
          9,  6,  8,  8,  8,  10, 9,  13, 11, 16, 16, 23, 18, 25, 20, 26, 22, 26, 23, 25, 23, 24 },
        { 5, 7, 4, 6, 5, 5, 6, 6, 6, 8, 5, 10, 4, 11 },
        { 11, 1, 9, 3, 7, 6, 5, 10, 4, 15, 4, 19, 5, 24, 7, 28, 9, 31, 11, 33 },
        { 3, 1, 5, 3, 7, 6, 9, 10, 10, 15, 10, 19, 9, 24, 7, 28, 5, 31, 3, 33 },
        { 8, 5, 8, 17, -1, -1, 3, 8, 13, 14, -1, -1, 13, 8, 3, 14 },
        { 13, 8, 13, 26, -1, -1, 4, 17, 22, 17 },
        { 6, 25, 5, 26, 4, 25, 5, 24, 6, 25, 6, -1, 5, 29, 4, 30 },
        { 4, 17, 22, 17 },
        { 5, 24, 4, 25, 5, 26, 6, 25, 5, 24 },
        { 20, 1, 2, 33 },
        { 9,  5,  6,  6,  4,  9,  3,  14, 3,  17, 4, 22, 6, 25, 9, 26, 11,
          26, 14, 25, 16, 22, 17, 17, 17, 14, 16, 9, 14, 6, 11, 5, 9,  5 },
        { 6, 9, 8, 8, 11, 5, 11, 26 },
        { 4,  10, 4,  9, 5,  7,  6,  6,  8,  5,  12, 5,  14, 6,
          15, 7,  16, 9, 16, 11, 15, 13, 13, 16, 3,  26, 17, 26 },
        { 5,  5,  16, 5,  10, 13, 13, 13, 15, 14, 16, 15, 17, 18, 17,
          20, 16, 23, 14, 25, 11, 26, 8,  26, 5,  25, 4,  24, 3,  22 },
        { 13, 5, 3, 19, 18, 19, -1, -1, 13, 5, 13, 26 },
        { 15, 5,  5,  5,  4,  14, 5,  13, 8,  12, 11, 12, 14, 13, 16, 15, 17,
          18, 17, 20, 16, 23, 14, 25, 11, 26, 8,  26, 5,  25, 4,  24, 3,  22 },
        { 16, 8,  15, 6,  12, 5,  10, 5,  7,  6,  5,  9,  4,  14, 4,  19,
          5,  23, 7,  25, 10, 26, 11, 26, 14, 25, 16, 23, 17, 20, 17, 19,
          16, 16, 14, 14, 11, 13, 10, 13, 7,  14, 5,  16, 4,  19 },
        { 17, 5, 7, 26, -1, -1, 3, 5, 17, 5 },
        { 8,  5,  5,  6,  4,  8,  4,  10, 5,  12, 7,  13, 11, 14, 14, 15, 16, 17, 17, 19,
          17, 22, 16, 24, 15, 25, 12, 26, 8,  26, 5,  25, 4,  24, 3,  22, 3,  19, 4,  17,
          6,  15, 9,  14, 13, 13, 15, 12, 16, 10, 16, 8,  15, 6,  12, 5,  8,  5 },
        { 16, 12, 15, 15, 13, 17, 10, 18, 9,  18, 6,  17, 4,  15, 3,  12, 3,  11, 4,  8, 6,  6, 9,
          5,  10, 5,  13, 6,  15, 8,  16, 12, 16, 17, 15, 22, 13, 25, 10, 26, 8,  26, 5, 25, 4, 23 },
        { 5, 12, 4, 13, 5, 14, 6, 13, 5, 12, -1, -1, 5, 24, 4, 25, 5, 26, 6, 25, 5, 24 },
        { 5, 12, 4, 13, 5, 14, 6, 13, 5, 12, -1, -1, 6, 25,
          5, 26, 4, 25, 5, 24, 6, 25, 6, -1, 5,  29, 4, 30 },
        { 20, 8, 4, 17, 20, 26 },
        { 4, 14, 22, 14, -1, -1, 4, 20, 22, 20 },
        { 4, 8, 20, 17, 4, 26 },
        { 3,  10, 3,  9,  4, 7,  5, 6,  7,  5,  11, 5,  13, 6,  14, 7,  15, 9,  15, 11,
          14, 13, 13, 14, 9, 16, 9, 19, -1, -1, 9,  24, 8,  25, 9,  26, 10, 25, 9,  24 },
        { 18, 13, 17, 11, 15, 10, 12, 10, 10, 11, 9,  12, 8,  15, 8,  18, 9,  20, 11, 21, 14, 21,
          16, 20, 17, 18, -1, -1, 12, 10, 10, 12, 9,  15, 9,  18, 10, 20, 11, 21, -1, -1, 18, 10,
          17, 18, 17, 20, 19, 21, 21, 21, 23, 19, 24, 16, 24, 14, 23, 11, 22, 9,  20, 7,  18, 6,
          15, 5,  12, 5,  9,  6,  7,  7,  5,  9,  4,  11, 3,  14, 3,  17, 4,  20, 5,  22, 7,  24,
          9,  25, 12, 26, 15, 26, 18, 25, 20, 24, 21, 23, -1, -1, 19, 10, 18, 18, 18, 20, 19, 21 },
        { 9, 5, 1, 26, -1, -1, 9, 5, 17, 26, -1, -1, 4, 19, 14, 19 },
        { 4,  5,  4,  26, -1, -1, 4,  5,  13, 5,  16, 6,  17, 7,  18, 9,
          18, 11, 17, 13, 16, 14, 13, 15, -1, -1, 4,  15, 13, 15, 16, 16,
          17, 17, 18, 19, 18, 22, 17, 24, 16, 25, 13, 26, 4,  26 },
        { 18, 10, 17, 8,  15, 6,  13, 5,  9, 5,  7,  6,  5,  8,  4,  10, 3,  13,
          3,  18, 4,  21, 5,  23, 7,  25, 9, 26, 13, 26, 15, 25, 17, 23, 18, 21 },
        { 4,  5,  4,  26, -1, -1, 4,  5,  11, 5,  14, 6,  16, 8, 17,
          10, 18, 13, 18, 18, 17, 21, 16, 23, 14, 25, 11, 26, 4, 26 },
        { 4, 5, 4, 26, -1, -1, 4, 5, 17, 5, -1, -1, 4, 15, 12, 15, -1, -1, 4, 26, 17, 26 },
        { 4, 5, 4, 26, -1, -1, 4, 5, 17, 5, -1, -1, 4, 15, 12, 15 },
        { 18, 10, 17, 8,  15, 6,  13, 5,  9,  5,  7,  6,  5,  8,  4,  10, 3,  13, 3,  18, 4,  21,
          5,  23, 7,  25, 9,  26, 13, 26, 15, 25, 17, 23, 18, 21, 18, 18, -1, -1, 13, 18, 18, 18 },
        { 4, 5, 4, 26, -1, -1, 18, 5, 18, 26, -1, -1, 4, 15, 18, 15 },
        { 4, 5, 4, 26 },
        { 12, 5, 12, 21, 11, 24, 10, 25, 8, 26, 6, 26, 4, 25, 3, 24, 2, 21, 2, 19 },
        { 4, 5, 4, 26, -1, -1, 18, 5, 4, 19, -1, -1, 9, 14, 18, 26 },
        { 4, 5, 4, 26, -1, -1, 4, 26, 16, 26 },
        { 4, 5, 4, 26, -1, -1, 4, 5, 12, 26, -1, -1, 20, 5, 12, 26, -1, -1, 20, 5, 20, 26 },
        { 4, 5, 4, 26, -1, -1, 4, 5, 18, 26, -1, -1, 18, 5, 18, 26 },
        { 9,  5,  7,  6,  5,  8,  4,  10, 3,  13, 3,  18, 4,  21, 5, 23, 7, 25, 9, 26, 13,
          26, 15, 25, 17, 23, 18, 21, 19, 18, 19, 13, 18, 10, 17, 8, 15, 6, 13, 5, 9,  5 },
        { 4, 5,  4, 26, -1, -1, 4,  5,  13, 5,  16, 6, 17,
          7, 18, 9, 18, 12, 17, 14, 16, 15, 13, 16, 4, 16 },
        { 9,  5,  7,  6,  5,  8,  4,  10, 3,  13, 3,  18, 4,  21, 5,  23,
          7,  25, 9,  26, 13, 26, 15, 25, 17, 23, 18, 21, 19, 18, 19, 13,
          18, 10, 17, 8,  15, 6,  13, 5,  9,  5,  -1, -1, 12, 22, 18, 28 },
        { 4,  5,  4,  26, -1, -1, 4,  5,  13, 5,  16, 6,  17, 7,  18, 9,
          18, 11, 17, 13, 16, 14, 13, 15, 4,  15, -1, -1, 11, 15, 18, 26 },
        { 17, 8,  15, 6,  12, 5,  8,  5,  5,  6,  3,  8,  3,  10, 4, 12, 5, 13, 7, 14,
          13, 16, 15, 17, 16, 18, 17, 20, 17, 23, 15, 25, 12, 26, 8, 26, 5, 25, 3, 23 },
        { 8, 5, 8, 26, -1, -1, 1, 5, 15, 5 },
        { 4, 5, 4, 20, 5, 23, 7, 25, 10, 26, 12, 26, 15, 25, 17, 23, 18, 20, 18, 5 },
        { 1, 5, 9, 26, -1, -1, 17, 5, 9, 26 },
        { 2, 5, 7, 26, -1, -1, 12, 5, 7, 26, -1, -1, 12, 5, 17, 26, -1, -1, 22, 5, 17, 26 },
        { 3, 5, 17, 26, -1, -1, 17, 5, 3, 26 },
        { 1, 5, 9, 15, 9, 26, -1, -1, 17, 5, 9, 15 },
        { 17, 5, 3, 26, -1, -1, 3, 5, 17, 5, -1, -1, 3, 26, 17, 26 },
        { 4, 1, 4, 33, -1, -1, 5, 1, 5, 33, -1, -1, 4, 1, 11, 1, -1, -1, 4, 33, 11, 33 },
        { 0, 5, 14, 29 },
        { 9, 1, 9, 33, -1, -1, 10, 1, 10, 33, -1, -1, 3, 1, 10, 1, -1, -1, 3, 33, 10, 33 },
        { 6, 11, 8, 8, 10, 11, -1, -1, 3, 14, 8, 9, 13, 14, -1, -1, 8, 9, 8, 26 },
        { 0, 28, 16, 28 },
        { 6, 5, 5, 6, 4, 8, 4, 10, 5, 11, 6, 10, 5, 9 },
        { 15, 12, 15, 26, -1, -1, 15, 15, 13, 13, 11, 12, 8,  12, 6,  13, 4,
          15, 3,  18, 3,  20, 4,  23, 6,  25, 8,  26, 11, 26, 13, 25, 15, 23 },
        { 4,  5,  4,  26, -1, -1, 4,  15, 6,  13, 8,  12, 11, 12, 13, 13, 15,
          15, 16, 18, 16, 20, 15, 23, 13, 25, 11, 26, 8,  26, 6,  25, 4,  23 },
        { 15, 15, 13, 13, 11, 12, 8, 12, 6,  13, 4,  15, 3,  18,
          3,  20, 4,  23, 6,  25, 8, 26, 11, 26, 13, 25, 15, 23 },
        { 15, 5, 15, 26, -1, -1, 15, 15, 13, 13, 11, 12, 8,  12, 6,  13, 4,
          15, 3, 18, 3,  20, 4,  23, 6,  25, 8,  26, 11, 26, 13, 25, 15, 23 },
        { 3,  18, 15, 18, 15, 16, 14, 14, 13, 13, 11, 12, 8,  12, 6,  13, 4,
          15, 3,  18, 3,  20, 4,  23, 6,  25, 8,  26, 11, 26, 13, 25, 15, 23 },
        { 10, 5, 8, 5, 6, 6, 5, 9, 5, 26, -1, -1, 2, 12, 9, 12 },
        { 15, 12, 15, 28, 14, 31, 13, 32, 11, 33, 8, 33, 6, 32, -1, -1, 15, 15, 13, 13, 11, 12,
          8,  12, 6,  13, 4,  15, 3,  18, 3,  20, 4, 23, 6, 25, 8,  26, 11, 26, 13, 25, 15, 23 },
        { 4, 5, 4, 26, -1, -1, 4, 16, 7, 13, 9, 12, 12, 12, 14, 13, 15, 16, 15, 26 },
        { 3, 5, 4, 6, 5, 5, 4, 4, 3, 5, -1, -1, 4, 12, 4, 26 },
        { 5, 5, 6, 6, 7, 5, 6, 4, 5, 5, -1, -1, 6, 12, 6, 29, 5, 32, 3, 33, 1, 33 },
        { 4, 5, 4, 26, -1, -1, 14, 12, 4, 22, -1, -1, 8, 18, 15, 26 },
        { 4, 5, 4, 26 },
        { 4,  12, 4,  26, -1, -1, 4,  16, 7,  13, 9,  12, 12, 12, 14, 13, 15, 16,
          15, 26, -1, -1, 15, 16, 18, 13, 20, 12, 23, 12, 25, 13, 26, 16, 26, 26 },
        { 4, 12, 4, 26, -1, -1, 4, 16, 7, 13, 9, 12, 12, 12, 14, 13, 15, 16, 15, 26 },
        { 8,  12, 6,  13, 4,  15, 3,  18, 3,  20, 4,  23, 6,  25, 8,  26, 11,
          26, 13, 25, 15, 23, 16, 20, 16, 18, 15, 15, 13, 13, 11, 12, 8,  12 },
        { 4,  12, 4,  33, -1, -1, 4,  15, 6,  13, 8,  12, 11, 12, 13, 13, 15,
          15, 16, 18, 16, 20, 15, 23, 13, 25, 11, 26, 8,  26, 6,  25, 4,  23 },
        { 15, 12, 15, 33, -1, -1, 15, 15, 13, 13, 11, 12, 8,  12, 6,  13, 4,
          15, 3,  18, 3,  20, 4,  23, 6,  25, 8,  26, 11, 26, 13, 25, 15, 23 },
        { 4, 12, 4, 26, -1, -1, 4, 18, 5, 15, 7, 13, 9, 12, 12, 12 },
        { 14, 15, 13, 13, 10, 12, 7,  12, 4,  13, 3,  15, 4,  17, 6,  18, 11,
          19, 13, 20, 14, 22, 14, 23, 13, 25, 10, 26, 7,  26, 4,  25, 3,  23 },
        { 5, 5, 5, 22, 6, 25, 8, 26, 10, 26, -1, -1, 2, 12, 9, 12 },
        { 4, 12, 4, 22, 5, 25, 7, 26, 10, 26, 12, 25, 15, 22, -1, -1, 15, 12, 15, 26 },
        { 2, 12, 8, 26, -1, -1, 14, 12, 8, 26 },
        { 3, 12, 7, 26, -1, -1, 11, 12, 7, 26, -1, -1, 11, 12, 15, 26, -1, -1, 19, 12, 15, 26 },
        { 3, 12, 14, 26, -1, -1, 14, 12, 3, 26 },
        { 2, 12, 8, 26, -1, -1, 14, 12, 8, 26, 6, 30, 4, 32, 2, 33, 1, 33 },
        { 14, 12, 3, 26, -1, -1, 3, 12, 14, 12, -1, -1, 3, 26, 14, 26 },
        { 9,  1,  7, 2,  6, 3,  5, 5,  5, 7,  6, 9,  7, 10, 8, 12, 8,  14, 6, 16,
          -1, -1, 7, 2,  6, 4,  6, 6,  7, 8,  8, 9,  9, 11, 9, 13, 8,  15, 4, 17,
          8,  19, 9, 21, 9, 23, 8, 25, 7, 26, 6, 28, 6, 30, 7, 32, -1, -1, 6, 18,
          8,  20, 8, 22, 7, 24, 6, 25, 5, -1, 5, 29, 6, 31, 7, 32, 9,  33 },
        { 4, 1, 4, 33 },
        { 5,  1,  7, 2,  8, 3,  9, 5,  9, 7,  8, 9,  7, 10, 6, 12, 6,  14, 8,  16,
          -1, -1, 7, 2,  8, 4,  8, 6,  7, 8,  6, 9,  5, 11, 5, 13, 6,  15, 10, 17,
          6,  19, 5, 21, 5, 23, 6, 25, 7, 26, 8, 28, 8, 30, 7, 32, -1, -1, 8,  18,
          6,  20, 6, 22, 7, 24, 8, 25, 9, -1, 9, 29, 8, 31, 7, 32, 5,  33 },
        { 3,  20, 3,  18, 4,  15, 6,  14, 8,  14, 10, 15, 14, 18, 16, 19,
          18, 19, 20, 18, 21, 16, -1, -1, 3,  18, 4,  16, 6,  15, 8,  15,
          10, 16, 14, 19, 16, 20, 18, 20, 20, 19, 21, 16, 21, 14 }
    };
}

namespace marengo {
namespace amaze {
namespace vectorfont {

std::vector<int> getCharacter(char c)
{
    unsigned int n = static_cast<int>(c) - 32;
    if (n >= characterLines.size()) {
        n = 0;
    }
    return characterLines[n];
}

} // namespace vectorfont
} // namespace amaze
} // namespace marengo
