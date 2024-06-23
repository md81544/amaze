#pragma once

#include <memory>
#include <vector>

#include "igraphicsadapter.h"

namespace marengo {
namespace amaze {
namespace helperfunctions {

void CsvSplit(const std::string& s, char c, std::vector<std::string>& v);
void CsvSplit(const std::string& s, char c, std::vector<double>& v);
double Sine(double degrees);
double Cosine(double degrees);
int Sgn(int x);
bool DoLinesIntersect(long x1, long y1, long x2, long y2, long x3, long y3, long x4, long y4);

} // namespace helperfunctions
} // namespace amaze
} // namespace marengo
