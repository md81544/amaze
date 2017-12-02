#include "gamemodel.h"
#include "helperfunctions.h"
#include "exceptions.h"

#include <boost/filesystem.hpp>

#include <sstream>
#include <numeric>

GameModel::GameModel()
{
    m_shipModel = std::make_shared<ShipModel>(
        ShipModel( NewGameShape(), NewGameShape(), NewGameShape() ) );
}

void GameModel::Initialise( size_t level_number )
{
    // Resets the model to a state ready for a new Level
    m_explosionIterationCount = 0;
    m_allDynamicGameShapes.clear();
    m_allStaticGameShapes.clear();

    CreateStaticShapes();

    // Grid lines on the background
    // graphicsManager_->setDrawColour(0, 100, 0, 255);
    std::shared_ptr<GameShape> bkg( new GameShape );
    for ( double n = 0; n <= 2000; n += 50 )
    {
        ShapeLine sl1{n, 0, n, 2000, 0, 100, 0, 255, 1};
        bkg->AddShapeLine( sl1 );
        ShapeLine sl2{0, n, 20000, n, 0, 100, 0, 255, 1};
        bkg->AddShapeLine( sl2 );
    }
    bkg->SetPos( 0, 0 );
    bkg->SetName( "Background" );
    bkg->SetGameShapeType( GameShapeType::NEUTRAL );
    m_allDynamicGameShapes.push_back( std::move( bkg ) );

    m_allDynamicGameShapes.push_back( m_shipModel->shipGameShape() );
    m_allDynamicGameShapes.push_back( m_shipModel->flamesGameShape() );
    m_allDynamicGameShapes.push_back( m_shipModel->explosionGameShape() );

    m_level = level_number;
    m_wasFuelOutWarned = false;
    m_wasTimeoutWarned = false;

    m_shipModel->initialise();
}

void GameModel::CreateStaticShapes()
{
    // Note static shapes' coodinates are expressed as if the viewport
    // is 320x240 pixels, regardless of the physical size of the window.

    std::shared_ptr<GameShape> gauge( new GameShape );
    ShapeLine sl1{0, 0, 6, 0, 64, 64, 64, 255, 1};
    gauge->AddShapeLine( sl1 );
    ShapeLine sl2{6, 0, 6, 220, 64, 64, 64, 255, 1};
    gauge->AddShapeLine( sl2 );
    ShapeLine sl3{6, 220, 0, 220, 64, 64, 64, 255, 1};
    gauge->AddShapeLine( sl3 );
    ShapeLine sl4{0, 220, 0, 0, 64, 64, 64, 255, 1};
    gauge->AddShapeLine( sl4 );
    gauge->SetPos( 10, 10 );
    m_allStaticGameShapes.push_back( gauge );

    m_staticInfoLine = std::make_shared<GameShape>();
    m_staticInfoLine->SetPos( 200, 10 );
    m_staticInfoLine->makeFromText( "Frame speed: 0 ms", 0, 175, 0, 255, 1 );
    m_staticInfoLine->SetScale( 0.25 );
    m_allStaticGameShapes.push_back( m_staticInfoLine );
}

int GameModel::Level() const { return m_level; }

void GameModel::SetLevel( int value ) { m_level = value; }

// TODO - put this in the controller?
void GameModel::LevelLoad( size_t levelNum )
{
    // The file format is as follows:
    // The first line should be "!~<timelimit>~<fuel>~<x>~<y>~" followed by
    // the Level title (Description)
    // Then each world object for the Level is described as follows:
    // Objects start with the line "N~<ObjectClass (text)>~<Unique ID>"
    // then the following can be used:
    // L~<x0>~<y0>~<x1>~<y1>~<R>~<G>~<B>~<Thickness (optional) - draws a line
    // P~<x>~<y> - set object's position
    // T~<text>~<R>~<G>~<B>~<Thickness (optional) - create object from text
    // C~<UniqueID> - copy the other object's shape
    //

    // All lines apply to the current object being described until a new
    // "N~..."
    // line is processed.

    // TODO: move the actual file access out into a separate class
    // or method

    Initialise( levelNum );

    std::string filename = "Level" + std::to_string( levelNum ) + ".cfg";
    boost::filesystem::path levelFile( GetDataPath() );
    levelFile.append( filename );

    std::ifstream in( levelFile.string() );
    if ( !in )
    {
        THROWUP( amaze::exceptions::AmazeRuntimeException,
            "Failed to load Level file " + filename );
    }
    std::string currentLine;
    std::unique_ptr<GameShape> obj( new GameShape );
    // TODO - object type : ObjectType cls = ObjectType::NEUTRAL;
    while ( !in.eof() )
    {
        getline( in, currentLine );
        std::vector<std::string> vec;
        HelperFunctions::CsvSplit( currentLine, '~', vec );
        // When we get here, vec is a vector of all the items on the current
        // line.
        if ( vec.size() == 0 )
        {
            continue;
        }
        char c = vec[ 0 ][ 0 ];
        long bestTime;
        switch ( c )
        {
        case '!': // timelimit, fuel, x, y, description
            m_timeLimit = stoi( vec[ 1 ] );
            bestTime = static_cast<long>( TimeGetTenthBest( levelNum ) );
            if ( m_timeLimit > bestTime )
            {
                m_timeLimit = bestTime;
            }
            m_shipModel->setFuel( stod( vec[ 2 ] ) );
            m_shipModel->setShipX( stod( vec[ 3 ] ) );
            m_shipModel->setShipY( stod( vec[ 4 ] ) );
            m_levelDescription = vec[ 5 ];
            break;
        case 'D':
            if ( vec.size() > 1 )
            {
                SetLevelDescription( vec[ 1 ] );
            }
            break;
        case 'N':
            if ( obj->GetGameShapeType() != GameShapeType::UNINITIALISED )
            {
                m_allDynamicGameShapes.push_back( std::move( obj ) );
            }
            obj = std::unique_ptr<GameShape>( new GameShape );
            obj->SetGameShapeType( GameShapeType::NEUTRAL );
            if ( vec.size() > 1 )
            {
                if ( vec[ 1 ] == "OBSTRUCTION" )
                {
                    obj->SetGameShapeType( GameShapeType::OBSTRUCTION );
                }
                else if ( vec[ 1 ] == "FUEL" )
                {
                    obj->SetGameShapeType( GameShapeType::FUEL );
                }
                else if ( vec[ 1 ] == "PRISONER" )
                {
                    obj->SetGameShapeType( GameShapeType::PRISONER );
                }
                else if ( vec[ 1 ] == "KEY" )
                {
                    obj->SetGameShapeType( GameShapeType::KEY );
                }
                else if ( vec[ 1 ] == "EXIT" )
                {
                    obj->SetGameShapeType( GameShapeType::EXIT );
                }
            }
            break;
        case 'L':
            if ( obj != nullptr && ( vec.size() == 8 || vec.size() == 9 ) )
            {
                ShapeLine sl1{
                    stod( vec[ 1 ] ),                         // x0
                    stod( vec[ 2 ] ),                         // y0
                    stod( vec[ 3 ] ),                         // x1
                    stod( vec[ 4 ] ),                         // y1
                    static_cast<uint8_t>( stoi( vec[ 5 ] ) ), // r
                    static_cast<uint8_t>( stoi( vec[ 6 ] ) ), // g
                    static_cast<uint8_t>( stoi( vec[ 7 ] ) ), // b
                    255,                                      // alpha
                    1                                         // thickness
                };
                if ( vec.size() == 9 )
                {
                    sl1.lineThickness = stoi( vec[ 8 ] );
                }
                obj->AddShapeLine( sl1 );
            }
            break;
        case 'P':
            if ( obj != nullptr && vec.size() == 3 )
            {
                obj->SetPos( stoi( vec[ 1 ] ), stoi( vec[ 2 ] ) );
            }
            break;
        case 'T':
            if ( obj != nullptr && vec.size() == 5 )
            {
                obj->makeFromText( vec[ 1 ], stoi( vec[ 2 ] ),
                    stoi( vec[ 3 ] ), stoi( vec[ 4 ] ), 255, 1 );
            }
            if ( obj != nullptr && vec.size() == 6 )
            {
                obj->makeFromText( vec[ 1 ], stoi( vec[ 2 ] ),
                    stoi( vec[ 3 ] ), stoi( vec[ 4 ] ), 255,
                    stoi( vec[ 5 ] ) );
            }
            break;
        case 'C':
            __debugbreak();
            //__asm("int $3"); // DEBUG TODO causes a debug break
            // if (obj != NULL && vec.size()>1) {
            //    obj=world.copyObject(dynamic_cast<Shape*>(mapObjects[vec[1]]),
            //    cls);
            //}
            break;
        default:
            break;
        }
        currentLine.clear();
    }
    in.close();
    if ( obj->GetGameShapeType() != GameShapeType::UNINITIALISED )
    {
        m_allDynamicGameShapes.push_back( std::move( obj ) );
    }
    SetLevel( levelNum );
}

void GameModel::SetDataPath( const std::string& dir ) { m_dataPath = dir; }

const std::string GameModel::GetDataPath() { return m_dataPath; }

double GameModel::TimeGetTenthBest( int levelNumber )
{
    return 300; // TODO
    /*
    std::vector<double> v;
    std::ostringstream ss;
    ss << "LEVEL" << levelNumber << "_BESTIMES";
    std::string s =
    prefs_.getString(ss.str(),"300,300,300,300,300,300,300,300,300,300");
    HelperFunctions::CsvSplit(s, ',' ,v);
    return v[9];
    */
}

std::string GameModel::LevelDescription() const { return m_levelDescription; }

void GameModel::SetLevelDescription( const std::string& value )
{
    m_levelDescription = value;
}

bool GameModel::GameIsRunning() const { return m_gameIsRunning; }

void GameModel::setGameIsRunning( bool value ) { m_gameIsRunning = value; }

std::shared_ptr<GameShape> GameModel::NewGameShape()
{
    std::shared_ptr<GameShape> gameShape( new GameShape );
    m_allDynamicGameShapes.push_back( gameShape );
    return ( gameShape );
}

const std::vector<std::shared_ptr<GameShape>>&
GameModel::GetAllDynamicObjects() const
{
    // TODO returning references to internals? Yuck.
    return m_allDynamicGameShapes;
}

const std::vector<std::shared_ptr<GameShape>>&
GameModel::GetAllStaticObjects() const
{
    // TODO returning references to internals? Yuck.
    return m_allStaticGameShapes;
}

std::shared_ptr<ShipModel> GameModel::ShipModel() const { return m_shipModel; }

bool GameModel::WasFuelOutWarned() const { return m_wasFuelOutWarned; }

void GameModel::SetWasFuelOutWarned( bool value )
{
    m_wasFuelOutWarned = value;
}

bool GameModel::WasTimeoutWarned() const { return m_wasTimeoutWarned; }

void GameModel::SetWasTimeoutWarned( bool value )
{
    m_wasTimeoutWarned = value;
}

int GameModel::TimeLimit() const { return m_timeLimit; }

void GameModel::SetTimeLimit( int value ) { m_timeLimit = value; }

std::shared_ptr<GameShape> GameModel::CollisionDetect() const
{
    for ( const auto& obj : m_allDynamicGameShapes )
    {
        if ( obj == m_shipModel->shipGameShape() )
        {
            // can't collide with itself :)
            continue;
        }
        if ( obj->GetGameShapeType() == GameShapeType::NEUTRAL )
        {
            // NEUTRAL objects don't cause collisions
            continue;
        }
        if ( obj->IsActive() == false )
        {
            continue;
        }
        if ( m_shipModel->shipGameShape()->IntersectCheck( obj ) )
        {
            return obj;
        }
    }
    return nullptr;
}

void GameModel::Process()
{
    if ( m_shipModel->isExploding() )
    {
        m_shipModel->setVisible( false );
        m_explosionIterationCount++;
        if ( m_explosionIterationCount > 50 )
        {
            setGameIsRunning( false );
        }
    }
    m_shipModel->process();

    std::ostringstream ss;
    ss << "Frame speed: " << m_averageFrameTime << " ms";
    m_staticInfoLine->makeFromText( ss.str(), 0, 175, 0, 255, 1 );
    m_staticInfoLine->SetScale( 0.25 );
}

void GameModel::UpdateStatistics( size_t millisecs )
{
    m_frameTimes.push_back( millisecs );
    if ( m_frameTimes.size() > 40 )
    {
        m_frameTimes.erase( m_frameTimes.begin() );
    }
    size_t sum;
    size_t count = m_frameTimes.size();
    sum = static_cast<size_t>(
        std::accumulate( m_frameTimes.begin(), m_frameTimes.end(), 0.0 ) );
    m_averageFrameTime = static_cast<size_t>( sum / count );
}
