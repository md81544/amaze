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
    void Initialise( size_t level_number );
    void LevelLoad( size_t levelNum );

    void SetDataPath( const std::string& dir );
    const std::string GetDataPath();

    void CreateStaticShapes();

    int Level() const;
    void SetLevel( int value );

    bool WasFuelOutWarned() const;
    void SetWasFuelOutWarned( bool value );

    bool WasTimeoutWarned() const;
    void SetWasTimeoutWarned( bool value );

    int TimeLimit() const;
    void SetTimeLimit( int value );

    double TimeGetTenthBest( int levelNumber );

    std::string LevelDescription() const;
    void SetLevelDescription( const std::string& value );

    bool GameIsRunning() const;
    void setGameIsRunning( bool value );

    std::shared_ptr<GameShape> NewGameShape();

    const std::vector<std::shared_ptr<GameShape>>&
    GetAllDynamicObjects() const;

    const std::vector<std::shared_ptr<GameShape>>&
    GetAllStaticObjects() const;

    std::shared_ptr<ShipModel> getShipModel() const;

    std::shared_ptr<GameShape> CollisionDetect() const;

    void Process();

    void UpdateStatistics( size_t millisecs );

    void ProcessDynamicObjects( std::function<void( GameShape& )> ) override;

    void ProcessStaticObjects( std::function<void( GameShape& )> ) override;

    unsigned int GetRotation() const override;

private:
    std::string m_dataPath { "" };

    int m_level{ -1 };
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

    std::shared_ptr<GameShape> m_staticInfoLine;

    std::shared_ptr<ShipModel> m_shipModel;

    std::vector<size_t> m_frameTimes;
    size_t m_averageFrameTime{ 0 };
};

} // namespace amaze
} // namespace marengo
