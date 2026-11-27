#include "levelprogress.h"

#include <algorithm>

namespace marengo {
namespace amaze {

LevelProgress::LevelProgress(
    std::vector<std::string> levels,
    std::vector<std::string> completed,
    std::optional<double> carriedShield,
    bool allCompleted)
    : m_levels(std::move(levels))
    , m_completed(std::move(completed))
    , m_carriedShield(carriedShield)
    , m_allCompleted(allCompleted)
{
}

bool LevelProgress::isListed(const std::string& level) const
{
    return std::ranges::find(m_levels, level) != m_levels.end();
}

bool LevelProgress::isCompleted(const std::string& level) const
{
    if (m_allCompleted && isListed(level)) {
        return true;
    }
    return std::ranges::find(m_completed, level) != m_completed.end();
}

bool LevelProgress::isUnlocked(const std::string& level) const
{
    return !isListed(level) || isCompleted(level) || nextLevel() == level;
}

std::optional<std::string> LevelProgress::nextLevel() const
{
    auto next = std::ranges::find_if(
        m_levels, [this](const std::string& level) { return !isCompleted(level); });
    if (next == m_levels.end()) {
        return std::nullopt;
    }
    return *next;
}

std::optional<std::string> LevelProgress::levelAfter(const std::string& level) const
{
    auto it = std::ranges::find(m_levels, level);
    if (it == m_levels.end() || std::next(it) == m_levels.end()) {
        return std::nullopt;
    }
    return *std::next(it);
}

double LevelProgress::startingShield(const std::string& level) const
{
    if (!isListed(level) || isCompleted(level)) {
        return 1.0;
    }
    return m_carriedShield.value_or(1.0);
}

bool LevelProgress::complete(const std::string& level, double shieldLeft)
{
    if (!isListed(level) || isCompleted(level)) {
        return false;
    }
    m_completed.push_back(level);
    m_carriedShield = std::clamp(shieldLeft, 0.0, 1.0);
    return true;
}

const std::vector<std::string>& LevelProgress::completedLevels() const
{
    return m_completed;
}

std::optional<double> LevelProgress::carriedShield() const
{
    return m_carriedShield;
}

} // namespace amaze
} // namespace marengo
