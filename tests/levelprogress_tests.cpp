#include "levelprogress.h"

#include <catch2/catch_test_macros.hpp>

using namespace marengo::amaze;

namespace {
const std::vector<std::string> LEVELS { "level0.lvl", "level1.lvl", "level2.lvl" };
}

TEST_CASE("At first, only the first level can be played")
{
    LevelProgress progress(LEVELS, {}, std::nullopt);
    CHECK(progress.nextLevel() == "level0.lvl");
    CHECK(progress.isUnlocked("level0.lvl"));
    CHECK_FALSE(progress.isUnlocked("level1.lvl"));
    CHECK_FALSE(progress.isUnlocked("level2.lvl"));
    CHECK_FALSE(progress.isCompleted("level0.lvl"));
    CHECK(progress.startingShield("level0.lvl") == 1.0);
}

TEST_CASE("Completing a level unlocks the next, which starts with the shield left")
{
    LevelProgress progress(LEVELS, {}, std::nullopt);
    CHECK(progress.complete("level0.lvl", 0.6));
    CHECK(progress.isCompleted("level0.lvl"));
    CHECK(progress.nextLevel() == "level1.lvl");
    CHECK(progress.isUnlocked("level1.lvl"));
    CHECK_FALSE(progress.isUnlocked("level2.lvl"));
    CHECK(progress.startingShield("level1.lvl") == 0.6);
    // Re-playing a completed level is with full shields
    CHECK(progress.startingShield("level0.lvl") == 1.0);
    CHECK(progress.completedLevels() == std::vector<std::string> { "level0.lvl" });
    CHECK(progress.carriedShield() == 0.6);
}

TEST_CASE("Completing a level again changes nothing")
{
    LevelProgress progress(LEVELS, { "level0.lvl" }, 0.6);
    CHECK_FALSE(progress.complete("level0.lvl", 1.0));
    CHECK(progress.carriedShield() == 0.6);
    CHECK(progress.completedLevels().size() == 1);
}

TEST_CASE("When every level is completed, there's no next level")
{
    LevelProgress progress(LEVELS, { "level2.lvl", "level0.lvl", "level1.lvl" }, 0.5);
    CHECK_FALSE(progress.nextLevel().has_value());
    for (const auto& level : LEVELS) {
        CHECK(progress.isUnlocked(level));
        CHECK(progress.startingShield(level) == 1.0);
    }
}

TEST_CASE("A level added before the last one completed becomes the next")
{
    // e.g. level1.lvl was added after level0 and level2 were completed
    LevelProgress progress(LEVELS, { "level0.lvl", "level2.lvl" }, 0.4);
    CHECK(progress.nextLevel() == "level1.lvl");
    CHECK(progress.isUnlocked("level2.lvl"));
    CHECK(progress.startingShield("level1.lvl") == 0.4);
}

TEST_CASE("The level after another is the next in order")
{
    LevelProgress progress(LEVELS, {}, std::nullopt);
    CHECK(progress.levelAfter("level0.lvl") == "level1.lvl");
    CHECK_FALSE(progress.levelAfter("level2.lvl").has_value());
    CHECK_FALSE(progress.levelAfter("elsewhere.lvl").has_value());
}

TEST_CASE("Levels which aren't in the list can be played, but aren't recorded")
{
    LevelProgress progress(LEVELS, {}, 0.3);
    CHECK(progress.isUnlocked("elsewhere.lvl"));
    CHECK(progress.startingShield("elsewhere.lvl") == 1.0);
    CHECK_FALSE(progress.complete("elsewhere.lvl", 0.5));
    CHECK(progress.completedLevels().empty());
}

TEST_CASE("Completed levels which are no longer listed are kept")
{
    LevelProgress progress(LEVELS, { "removed.lvl" }, std::nullopt);
    CHECK(progress.nextLevel() == "level0.lvl");
    progress.complete("level0.lvl", 1.0);
    CHECK(progress.completedLevels() == std::vector<std::string> { "removed.lvl", "level0.lvl" });
}

TEST_CASE("With all levels treated as completed, everything is open and nothing is recorded")
{
    LevelProgress progress(LEVELS, {}, 0.2, true);
    CHECK_FALSE(progress.nextLevel().has_value());
    for (const auto& level : LEVELS) {
        CHECK(progress.isUnlocked(level));
        CHECK(progress.isCompleted(level));
        CHECK(progress.startingShield(level) == 1.0);
    }
    CHECK_FALSE(progress.complete("level1.lvl", 0.5));
    CHECK(progress.completedLevels().empty());
    CHECK(progress.carriedShield() == 0.2);
}
