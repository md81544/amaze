#pragma once

// The player's progress through the levels, which are played in order. A
// level can be played once it's been completed, or if it's the next one (the
// first which hasn't been). Completing a new level carries the shield the
// player had left over to the next one; levels which have already been
// completed are re-played with full shields.
//
// Levels are identified by their file names, without directories (e.g.
// "level3.lvl"), so progress survives the game being moved. Levels which
// aren't in the list (e.g. loaded from elsewhere with --file) can always be
// played, with full shields, and completing them isn't recorded.

#include <optional>
#include <string>
#include <vector>

namespace marengo {
namespace amaze {

class LevelProgress {
public:
    // levels: in the order they're played. completed: in any order; any which
    // aren't in levels are kept, in case they come back. allCompleted treats
    // every level as completed (for testing levels), without recording it.
    LevelProgress(
        std::vector<std::string> levels,
        std::vector<std::string> completed,
        std::optional<double> carriedShield,
        bool allCompleted = false);

    bool isCompleted(const std::string& level) const;
    // Whether the level can be played
    bool isUnlocked(const std::string& level) const;
    // The first level which hasn't been completed, if any
    std::optional<std::string> nextLevel() const;
    // The level after this one, if any
    std::optional<std::string> levelAfter(const std::string& level) const;

    // The shield (from 0 to 1) the level starts with
    double startingShield(const std::string& level) const;
    // Records that the level has been completed, with the shield left.
    // Returns whether it was a new level (and so the progress has changed).
    bool complete(const std::string& level, double shieldLeft);

    // For saving
    const std::vector<std::string>& completedLevels() const;
    std::optional<double> carriedShield() const;

private:
    bool isListed(const std::string& level) const;

    std::vector<std::string> m_levels;
    std::vector<std::string> m_completed;
    std::optional<double> m_carriedShield;
    bool m_allCompleted;
};

} // namespace amaze
} // namespace marengo
