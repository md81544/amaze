#pragma once

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

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

// Simulation rate. All per-tick values (physics, schedule durations, etc.) are
// tuned for this rate.
constexpr int TICKS_PER_SECOND = 120;

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

// A piece of debris falling into a gravity object, to show its field
struct GravityDebris {
    QPointF pos; // relative to the object's centre
    QPointF velocity; // units per tick
    int age { 0 }; // ticks since it appeared
};

struct GravityField {
    std::shared_ptr<GameShape> shape;
    std::vector<GravityDebris> debris;
};

class GameModel final {
public:
    explicit GameModel(const std::string& dataPath);

    // Reset the model to a state ready for a new Level:
    void initialise(const std::string& levelFileName);
    void levelLoad(const std::string& filename);

    const std::string getDataPath();

    void buildBreakableExplosionShape();

    std::string levelFileName() const;
    void setLevelFileName(const std::string& filename);

    std::string levelDescription() const;
    void setLevelDescription(const std::string& value);
    // Shown before the level starts instead of "Get Ready", if not empty:
    // plain text or HTML (see LEVEL_FORMAT.md)
    const std::string& levelIntroduction() const;

    // The ship is replaced when a level loads, so don't keep this reference
    // beyond the current tick
    ShipModel& shipModel();
    const ShipModel& shipModel() const;

    std::tuple<GameShapeType, std::shared_ptr<GameShape>> collisionDetect() const;
    // Bounces the ship off the obstacle(s) it's touching
    Impact bounceShip();

    // Advances special objects, the ship, explosions etc. by one simulation tick
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
    // e.g. the shield carried over from the previous level
    void setShield(double value);

    // The pull on the ship (as a change in its velocity per tick) of the
    // strongest gravity affecting it, or 0 if none is
    double gravitationalPull() const;

    // Debris falling into each object with gravity
    const std::vector<GravityField>& gravityFields() const;

private:
    std::string m_dataPath { "" };

    std::string m_levelFileName;
    std::string m_levelDescription;
    std::string m_levelIntroduction;

    // Container for all GameShapes in the game:
    std::vector<std::shared_ptr<GameShape>> m_allDynamicGameShapes;

    std::unique_ptr<ShipModel> m_shipModel;

    std::vector<GravityField> m_gravityFields;
    // Sets up the debris for the level's objects with gravity
    void createGravityFields();

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
