#pragma once

#include <vector>
#include <memory>

#include "igraphicsadapter.h"

namespace marengo
{
namespace amaze
{


void CsvSplit( const std::string& s, char c, std::vector<std::string>& v );
void CsvSplit( const std::string& s, char c, std::vector<double>& v );
double Sine( int degrees );
double Cosine( int degrees );
int Sgn( int x );
bool DoLinesIntersect(
    long x1, long y1, long x2, long y2, long x3, long y3, long x4, long y4 );

}

} // namespace amaze
} // namespace marengo
