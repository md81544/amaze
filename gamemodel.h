#pragma once

#include <memory>

#include "gameshape.h"
#include "imodel.h"
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

class GameModel : public IModel {
public:
    GameModel();

    // Reset the model to a state ready for a new Level:
    void initialise(size_t level_number);
    void levelLoad(size_t levelNum);

    void setDataPath(const std::string& dir);
    const std::string getDataPath();

    void createStaticShapes();

    size_t level() const;
    void setLevel(size_t value);

    bool wasTimeoutWarned() const;
    void setWasTimeoutWarned(bool value);

    std::string levelDescription() const;
    void setLevelDescription(const std::string& value);

    bool gameIsRunning() const;
    void setGameIsRunning(bool value);

    std::shared_ptr<GameShape> newGameShape();

    // Returns a non-owning pointer to the ship model
    ShipModel* getShipModel() const;

    std::shared_ptr<GameShape> collisionDetect() const;

    void process();

    void processDynamicObjects(std::function<void(GameShape&)>) override;

    void processStaticObjects(std::function<void(GameShape&)>) override;

    unsigned int getRotation() const override;

    void togglePause();
    bool getPausedState(); // returns true if paused

private:
    std::string m_dataPath { "" };

    size_t m_level { 0 };
    std::string m_levelDescription;
    bool m_gameIsRunning { false };

    int m_explosionIterationCount { 0 };

    // Container for all GameShapes in the game:
    std::vector<std::shared_ptr<GameShape>> m_allDynamicGameShapes;

    // Container for all "static" items - these are items which do not rotate
    // and stay in  on screen
    std::vector<std::shared_ptr<GameShape>> m_allStaticGameShapes;

    std::unique_ptr<ShipModel> m_shipModel;

    size_t m_averageFrameTime { 0 };
    std::shared_ptr<GameShape> m_pauseMessage;

    bool m_paused{false};
};

} // namespace amaze
} // namespace marengo
