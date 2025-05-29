#pragma once

#include <memory>
#include <vector>

#include "igraphicsadapter.h"

namespace marengo {
namespace amaze {
namespace helperfunctions {

void csvSplit(const std::string& s, char c, std::vector<std::string>& v);
void csvSplit(const std::string& s, char c, std::vector<double>& v);
double sine(double degrees);
double cosine(double degrees);
int sgn(double x);
bool doLinesIntersect(long x1, long y1, long x2, long y2, long x3, long y3, long x4, long y4);

template <typename T, std::size_t N> class RingBuffer {
public:
    RingBuffer()
    {
        clear();
    }

    void add(const T& item)
    {
        m_buffer[m_head] = item;
        advance(m_head);
        if (m_head == m_tail) {
            advance(m_tail);
        }
    }

    const T& lastItem()
    {
        return m_buffer[m_tail];
    }

    void clear()
    {
        m_tail = m_head = 0;
    }

private:
    static const std::size_t BUFSIZE = N + 1;

    void advance(std::size_t& ptr)
    {
        ptr = (ptr + 1) % BUFSIZE;
    }

    T m_buffer[BUFSIZE];
    std::size_t m_head;
    std::size_t m_tail;
};

} // namespace helperfunctions
} // namespace amaze
} // namespace marengo
