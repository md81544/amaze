#pragma once

#include <memory>
#include <string>

#include "gameshape.h"
#include "helperfunctions.h"
#include "imodel.h"
#include "scheduler.h"
#include "shipmodel.h"

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
    Running, // normal operation
    Exploding, // ship is exploding
    Dead, // life is lost
    Succeeded, // found the exit
    Paused, // game is paused
    Quit, // user requested to quit
    Menu  // Level selection / options etc
};

struct ShipPosition {
    unsigned int posX;
    unsigned int posY;
    double rotation;
};

// This is only used for "static" text, i.e. text which doesn't move/rotate, e.g. lives left
struct Text {
    std::string text;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    unsigned characterSize;
    float positionX;
    float positionY;
};

class GameModel : public IModel {
public:
    GameModel();

    // Reset the model to a state ready for a new Level:
    void initialise(size_t level_number);
    void levelLoad(size_t levelNum);

    void setDataPath(const std::string& dir);
    const std::string getDataPath();

    void createStaticShapes();
    void buildBreakableExplosionShape();

    size_t level() const;
    void setLevel(size_t value);

    std::string levelDescription() const;
    void setLevelDescription(const std::string& value);

    std::shared_ptr<GameShape> newGameShape();

    // Returns a non-owning pointer to the ship model
    ShipModel* getShipModel() const;

    std::shared_ptr<GameShape> collisionDetect() const;

    void process();
    void processDynamicObjects(std::function<void(GameShape&)>) override;
    void processStaticObjects(std::function<void(GameShape&)>) override;
    unsigned int getRotation() const override;
    // Save rocket position periodically to facilitate a restart after a life is used
    void savePosition();
    void togglePause();
    bool getPausedState(); // returns true if paused
    void restart();
    GameState getGameState();
    void setGameState(GameState state);
    int lifeLost();
    void extraLife();
    void setBreakableExploding(bool value = true);

private:
    std::string m_dataPath { "" };

    size_t m_level { 0 };
    std::string m_levelDescription;

    // Container for all GameShapes in the game:
    std::vector<std::shared_ptr<GameShape>> m_allDynamicGameShapes;

    // Container for all "static" items - these are items which do not rotate
    // and stay in  on screen
    std::vector<std::shared_ptr<GameShape>> m_allStaticGameShapes;

    std::unique_ptr<ShipModel> m_shipModel;

    size_t m_averageFrameTime { 0 };
    std::shared_ptr<GameShape> m_pauseMessage;
    std::shared_ptr<GameShape> m_livesRemainingLabel;
    std::shared_ptr<GameShape> m_breakableExplosionShape;
    bool m_breakableExploding { false };

    helperfunctions::RingBuffer<ShipPosition, 200> m_savedPositionsRingBuffer;
    GameState m_gameState { GameState::Running };
    void rebuildShip();
    int m_livesRemaining { 1 };
    void setLivesRemainingText();
    Scheduler m_scheduler;

};

} // namespace amaze
} // namespace marengo
