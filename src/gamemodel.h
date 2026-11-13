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
    Dead, // life is lost
    Succeeded, // found the exit
    Paused, // game is paused
    Quit, // user requested to quit
    Menu // Level selection / options etc
};

struct ShipPosition {
    unsigned int posX;
    unsigned int posY;
    double rotation;
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

    std::shared_ptr<GameShape> newGameShape();

    // The ship is replaced when a level loads or the ship respawns, so don't
    // keep this reference beyond the current tick
    ShipModel& shipModel();
    const ShipModel& shipModel() const;

    std::tuple<GameShapeType, std::shared_ptr<GameShape>> collisionDetect() const;
    // Bounces the ship off the obstacle(s) it's touching. Returns the speed of
    // impact, i.e. how fast the ship was moving towards the obstacle.
    double bounceShip();

    // Advances moving objects, the ship, explosions etc. by one simulation tick
    void tick();
    const std::vector<std::shared_ptr<GameShape>>& getAllDynamicObjects() const;

    // Save rocket position periodically to facilitate a restart after a life is used
    void savePosition();
    void togglePause();
    bool gameIsPaused(); // returns true if paused
    void restart();
    GameState getGameState();
    void setGameState(GameState state);
    int lifeLost();
    void extraLife();
    void setLives(int lives);
    void setBreakableExploding(bool value = true);
    int getLivesRemaining()
    {
        return m_livesRemaining;
    };

private:
    std::string m_dataPath { "" };

    std::string m_levelFileName;
    std::string m_levelDescription;

    // Container for all GameShapes in the game:
    std::vector<std::shared_ptr<GameShape>> m_allDynamicGameShapes;

    std::unique_ptr<ShipModel> m_shipModel;

    std::shared_ptr<GameShape> m_breakableExplosionShape;
    bool m_breakableExploding { false };

    utils::RingBuffer<ShipPosition, 200> m_savedPositionsRingBuffer;
    GameState m_gameState { GameState::Running };
    void rebuildShip();
    // Combined contact normal for all obstacles the ship is touching (see
    // Shape::contactNormal())
    std::optional<QPointF> shipContactNormal() const;
    bool shipTouchesObstacle() const;
    // Moves the ship away from obstacles it's touching, until it's clear or
    // has moved a limited distance. Returns true if it's clear.
    bool pushShipClear();
    int m_livesRemaining { 1 };
    Scheduler m_scheduler;
};

} // namespace amaze
} // namespace marengo
