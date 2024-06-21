#pragma once

#include <memory>

#include "imodel.h"
#include "gameshape.h"
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

namespace marengo
{
namespace amaze
{

class GameModel : public IModel
{
public:
    GameModel();

    // Reset the model to a state ready for a new Level:
    void initialise( size_t level_number );
    void levelLoad( size_t levelNum );

    void setDataPath( const std::string& dir );
    const std::string getDataPath();

    void createStaticShapes();

    size_t level() const;
    void setLevel( size_t value );

    bool wasFuelOutWarned() const;
    void setWasFuelOutWarned( bool value );

    bool wasTimeoutWarned() const;
    void setWasTimeoutWarned( bool value );

    int timeLimit() const;
    void setTimeLimit( int value );

    double timeGetTenthBest( size_t levelNumber );

    std::string levelDescription() const;
    void setLevelDescription( const std::string& value );

    bool gameIsRunning() const;
    void setGameIsRunning( bool value );

    std::shared_ptr<GameShape> newGameShape();

    const std::vector<std::shared_ptr<GameShape>>&
    getAllDynamicObjects() const;

    const std::vector<std::shared_ptr<GameShape>>&
    getAllStaticObjects() const;

    // Returns a non-owning pointer to the ship model
    ShipModel* getShipModel() const;

    std::shared_ptr<GameShape> collisionDetect() const;

    void process();

    void updateStatistics( size_t millisecs );

    void processDynamicObjects( std::function<void( GameShape& )> ) override;

    void processStaticObjects( std::function<void( GameShape& )> ) override;

    unsigned int getRotation() const override;

private:
    std::string m_dataPath { "" };

    size_t m_level{ 0 };
    std::string m_levelDescription;
    int m_timeLimit;
    bool m_wasFuelOutWarned{ false };
    bool m_wasTimeoutWarned{ false };
    bool m_gameIsRunning{ false };

    int m_explosionIterationCount{0};

    // Container for all GameShapes in the game:
    std::vector<std::shared_ptr<GameShape>> m_allDynamicGameShapes;

    // Container for all "static" items - these are items which do not rotate
    // and stay in  on screen, e.g. fuel gauge and also, the "display"
    // version of the ship.
    std::vector<std::shared_ptr<GameShape>> m_allStaticGameShapes;

    // std::shared_ptr<GameShape> m_staticInfoLine;

    std::unique_ptr<ShipModel> m_shipModel;

    std::vector<size_t> m_frameTimes;
    size_t m_averageFrameTime{ 0 };
};

} // namespace amaze
} // namespace marengo
