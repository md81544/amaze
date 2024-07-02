#include "gamemodel.h"
#include "exceptions.h"
#include "helperfunctions.h"

#include <cassert>
#include <fstream>
#include <numeric>
#include <sstream>

namespace marengo {
namespace amaze {

GameModel::GameModel()
{
    m_shipModel
        = std::make_unique<ShipModel>(ShipModel(newGameShape(), newGameShape(), newGameShape()));
    m_pauseMessage = std::make_shared<GameShape>();
}

void GameModel::initialise(size_t levelNumber)
{
    // Resets the model to a state ready for a new Level
    m_explosionIterationCount = 0;
    m_allDynamicGameShapes.clear();
    m_allStaticGameShapes.clear();

    createStaticShapes();

    // Grid lines on the background
    std::shared_ptr<GameShape> bkg(new GameShape);
    for (double n = 0; n <= 2000; n += 50) {
        ShapeLine sl1 { n, 0, n, 2000, 0, 100, 0, 255, 1 };
        bkg->AddShapeLine(sl1);
        ShapeLine sl2 { 0, n, 2000, n, 0, 100, 0, 255, 1 };
        bkg->AddShapeLine(sl2);
    }
    bkg->SetPos(0, 0);
    bkg->SetName("Background");
    bkg->SetGameShapeType(GameShapeType::NEUTRAL);
    m_allDynamicGameShapes.push_back(std::move(bkg));

    m_allDynamicGameShapes.push_back(m_shipModel->shipGameShape());
    m_allDynamicGameShapes.push_back(m_shipModel->flamesGameShape());
    m_allDynamicGameShapes.push_back(m_shipModel->explosionGameShape());

    m_level = levelNumber;

    m_shipModel->initialise();
}

void GameModel::createStaticShapes()
{
    // Note static shapes' coodinates are expressed as if the viewport
    // is 480 pixels wide, regardless of its actual size
    m_pauseMessage->makeFromText("PAUSED", 241, 252, 33, 255, 6);
    m_pauseMessage->SetPos(240, 100);
    m_pauseMessage->SetVisible(false);
    m_pauseMessage->SetName("gameModel::m_pauseMessage"); // useful for debugging only
    m_allStaticGameShapes.push_back(m_pauseMessage);
}

size_t GameModel::level() const
{
    return m_level;
}

void GameModel::setLevel(size_t value)
{
    m_level = value;
}

// TODO - put this in the controller?
void GameModel::levelLoad(size_t levelNum)
{
    // TODO convert this format to YAML or JSON?
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

    initialise(levelNum);

    std::string filename = "level" + std::to_string(levelNum) + ".cfg";
    std::filesystem::path levelFile(getDataPath());
    levelFile.append(filename);

    std::ifstream in(levelFile.string());
    if (!in) {
        THROWUP(AmazeRuntimeException, "Failed to load Level file " + filename);
    }
    std::string currentLine;
    std::unique_ptr<GameShape> obj(new GameShape);
    // TODO - object type : ObjectType cls = ObjectType::NEUTRAL;
    while (!in.eof()) {
        getline(in, currentLine);
        std::vector<std::string> vec;
        helperfunctions::CsvSplit(currentLine, '~', vec);
        // When we get here, vec is a vector of all the items on the current
        // line.
        if (vec.size() == 0) {
            continue;
        }
        char c = vec[0][0];
        switch (c) {
            case '!': // timelimit (unused), fuel (unused), ship x, ship y, description (unused)
                m_shipModel->setShipX(stod(vec[3]));
                m_shipModel->setShipY(stod(vec[4]));
                m_levelDescription = vec[5];
                break;
            case 'N': // New object, parameter 1 is type, parameter 2 appears unused
                if (obj->GetGameShapeType() != GameShapeType::UNINITIALISED) {
                    m_allDynamicGameShapes.push_back(std::move(obj));
                }
                obj = std::unique_ptr<GameShape>(new GameShape);
                obj->SetGameShapeType(GameShapeType::NEUTRAL);
                if (vec.size() > 1) {
                    if (vec[1] == "OBSTRUCTION") {
                        obj->SetGameShapeType(GameShapeType::OBSTRUCTION);
                    } else if (vec[1] == "FUEL") {
                        obj->SetGameShapeType(GameShapeType::FUEL);
                    } else if (vec[1] == "PRISONER") {
                        obj->SetGameShapeType(GameShapeType::PRISONER);
                    } else if (vec[1] == "KEY") {
                        obj->SetGameShapeType(GameShapeType::KEY);
                    } else if (vec[1] == "EXIT") {
                        obj->SetGameShapeType(GameShapeType::EXIT);
                    }
                }
                break;
            case 'L':
                if (obj != nullptr && (vec.size() == 8 || vec.size() == 9)) {
                    ShapeLine sl1 {
                        stod(vec[1]), // x0
                        stod(vec[2]), // y0
                        stod(vec[3]), // x1
                        stod(vec[4]), // y1
                        static_cast<uint8_t>(stoi(vec[5])), // r
                        static_cast<uint8_t>(stoi(vec[6])), // g
                        static_cast<uint8_t>(stoi(vec[7])), // b
                        255, // alpha
                        stoi(vec[5]) // thickness
                    };
                    if (vec.size() == 9) {
                        sl1.lineThickness = stoi(vec[8]);
                    }
                    obj->AddShapeLine(sl1);
                }
                break;
            case 'P':
                if (obj != nullptr && vec.size() == 3) {
                    obj->SetPos(stoi(vec[1]), stoi(vec[2]));
                }
                break;
            case 'T':
                if (obj != nullptr && vec.size() == 5) {
                    obj->makeFromText(vec[1], stoi(vec[2]), stoi(vec[3]), stoi(vec[4]), 255, 6);
                }
                if (obj != nullptr && vec.size() == 6) {
                    obj->makeFromText(
                        vec[1], stoi(vec[2]), stoi(vec[3]), stoi(vec[4]), 255, stoi(vec[5]));
                }
                break;
            case 'C':
                // Think this is currently unused...
                assert(false);
                break;
            default:
                break;
        }
        currentLine.clear();
    }
    in.close();
    if (obj->GetGameShapeType() != GameShapeType::UNINITIALISED) {
        m_allDynamicGameShapes.push_back(std::move(obj));
    }
    setLevel(levelNum);
}

void GameModel::setDataPath(const std::string& dir)
{
    m_dataPath = dir;
}

const std::string GameModel::getDataPath()
{
    return m_dataPath;
}

std::string GameModel::levelDescription() const
{
    return m_levelDescription;
}

void GameModel::setLevelDescription(const std::string& value)
{
    m_levelDescription = value;
}

bool GameModel::gameIsRunning() const
{
    return m_gameIsRunning;
}

void GameModel::setGameIsRunning(bool value)
{
    m_gameIsRunning = value;
}

std::shared_ptr<GameShape> GameModel::newGameShape()
{
    std::shared_ptr<GameShape> gameShape(new GameShape);
    m_allDynamicGameShapes.push_back(gameShape);
    return (gameShape);
}

ShipModel* GameModel::getShipModel() const
{
    // TODO returning pointer to internals? Yuck.
    // replace this with member functions which internally
    // reference m_shipModel.
    return m_shipModel.get();
}

std::shared_ptr<GameShape> GameModel::collisionDetect() const
{
    for (const auto& obj : m_allDynamicGameShapes) {
        if (obj == m_shipModel->shipGameShape()) {
            // can't collide with itself :)
            continue;
        }
        if (obj->GetGameShapeType() == GameShapeType::NEUTRAL) {
            // NEUTRAL objects don't cause collisions
            continue;
        }
        if (obj->IsActive() == false) {
            continue;
        }
        if (m_shipModel->shipGameShape()->IntersectCheck(obj)) {
            return obj;
        }
    }
    return nullptr;
}

void GameModel::process() // TODO more descriptive name
{
    if (m_shipModel->isExploding()) {
        m_shipModel->setVisible(false);
        m_explosionIterationCount++;
        if (m_explosionIterationCount > 150) {
            setGameIsRunning(false);
        }
    }
    m_shipModel->process();
}

void GameModel::processDynamicObjects(std::function<void(GameShape&)> process)
{
    for (const auto& shape : m_allDynamicGameShapes) {
        process(*shape);
    }
}

void GameModel::processStaticObjects(std::function<void(GameShape&)> process)
{
    for (const auto& shape : m_allStaticGameShapes) {
        process(*shape);
    }
}

unsigned int GameModel::getRotation() const
{
    return 0; // TODO
}

void GameModel::togglePause()
{
    m_paused = !m_paused;
    m_pauseMessage->SetVisible(m_paused);
}

bool GameModel::getPausedState()
{
    return m_paused;
}

} // namespace amaze
} // namespace marengo
