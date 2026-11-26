#pragma once

#include <memory>
#include <optional>
#include <string>
#include <tuple>

#include "gameshape.h"
#include "scheduler.h"
#include "shipmodel.h"
#include "utils.h"

// This class acts as the MVC "model" for the game.
// Note other models exist and are aggredated here
// (e.g. the shipModel)

// All "GameShape" objects are stored in this object.

// Note the "arena" is arbitrarily 2000x2000 "units"
// in dimension. The viewport is 320x240 of these units.
// The View model displays the viewport at the size of
// the physical window and scales the "units" to pixels
// accordingly.

namespace marengo {
namespace amaze {

enum class GameState {
    GetReady, // level loaded, about to start
    Running, // normal operation
    Exploding, // ship is exploding
    Dead, // ship has been destroyed: game over
    Succeeded, // found the exit
    Paused, // game is paused
    Quit, // user requested to quit
    Menu // Level selection / options etc
};

struct Impact {
    // How fast the part of the ship which was hit was moving towards the
    // obstacle
    double speed { 0.0 };
    // The ship had to be pushed clear of the obstacle, e.g. after rotating
    // into it
    bool pushed { false };
};

class GameModel final {
public:
    explicit GameModel(const std::string& dataPath);

    // Reset the model to a state ready for a new Level:
    void initialise(const std::string& levelFileName);
    void levelLoad(size_t levelNum);
    void levelLoad(const std::string& filename);

    const std::string getDataPath();

    void buildBreakableExplosionShape();

    std::string levelFileName() const;
    void setLevelFileName(const std::string& filename);

    std::string levelDescription() const;
    void setLevelDescription(const std::string& value);

    // The ship is replaced when a level loads, so don't keep this reference
    // beyond the current tick
    ShipModel& shipModel();
    const ShipModel& shipModel() const;

    std::tuple<GameShapeType, std::shared_ptr<GameShape>> collisionDetect() const;
    // Bounces the ship off the obstacle(s) it's touching
    Impact bounceShip();

    // Advances moving objects, the ship, explosions etc. by one simulation tick
    void tick();
    const std::vector<std::shared_ptr<GameShape>>& getAllDynamicObjects() const;

    void togglePause();
    bool gameIsPaused(); // returns true if paused
    GameState getGameState();
    void setGameState(GameState state);
    void setBreakableExploding(bool value = true);

    // The ship's shield, from 1 (full) down to 0, when the ship is destroyed
    double shield() const;
    // Weakens the shield according to the speed of an impact. Being pushed
    // clear of an obstacle counts as at least a graze.
    void shieldImpact(const Impact& impact);
    void rechargeShield(double amount);

private:
    std::string m_dataPath { "" };

    std::string m_levelFileName;
    std::string m_levelDescription;

    // Container for all GameShapes in the game:
    std::vector<std::shared_ptr<GameShape>> m_allDynamicGameShapes;

    std::unique_ptr<ShipModel> m_shipModel;

    std::shared_ptr<GameShape> m_breakableExplosionShape;
    bool m_breakableExploding { false };

    GameState m_gameState { GameState::Running };
    // Combined contact with all obstacles the ship is touching (see
    // Shape::contact())
    std::optional<Shape::Contact> shipContact() const;
    bool shipTouchesObstacle() const;
    // Moves the ship away from obstacles it's touching, until it's clear or
    // has moved a limited distance. Returns true if it's clear.
    bool pushShipClear();
    double m_shield { 1.0 };
    // So that an impact (which may include a few smaller bounces) only
    // weakens the shield once: the largest recent damage, and how long ago
    double m_recentShieldDamage { 0.0 };
    int m_ticksSinceShieldDamage { 0 };
    Scheduler m_scheduler;
};

} // namespace amaze
} // namespace marengo
