#include "gamemodel.h"
#include "exceptions.h"
#include "log.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <numeric>
#include <regex>
#include <sstream>
#include <thread>

namespace marengo {
namespace amaze {

namespace {

std::string getLevelDescription(std::filesystem::path levelFile)
{
    std::ifstream in(levelFile.string());
    if (!in) {
        THROWUP(AmazeRuntimeException, "Failed to load Level file " + levelFile.string());
    }
    std::string currentLine;
    std::string levelDescription { "Untitled" };
    getline(in, currentLine);
    std::vector<std::string> vec;
    helperfunctions::csvSplit(currentLine, '~', vec);
    if (vec.size() > 6) {
        levelDescription = vec[6];
    }
    in.close();
    return levelDescription;
}

} // namespace

GameModel::GameModel(const std::string& dataPath)
    : m_dataPath(dataPath)
{
    m_shipModel
        = std::make_unique<ShipModel>(ShipModel(newGameShape(), newGameShape(), newGameShape()));

    // Populate the menu structure
    m_menu.addMenuItem(
        "Main Menu", { "Main Menu", MenuItemId::LEVEL_SELECT, "Select Level", 0, "Select Level" });
    m_menu.addMenuItem("Main Menu", { "Main Menu", MenuItemId::OPTIONS, "Options", 1 });
    m_menu.addMenuItem("Main Menu", { "Main Menu", MenuItemId::QUIT, "Quit", 2 });

    // Sub-menu for level selection
    std::regex regexPattern("^level.*cfg$");
    std::map<int, std::filesystem::path> sortedPaths;
    for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
        if (entry.is_regular_file()) {
            const std::string fileName = entry.path().filename().string();
            if (std::regex_match(fileName, regexPattern)) {
                std::string num;
                for (const auto c : fileName) {
                    if (isdigit(c)) {
                        num += c;
                    }
                }
                sortedPaths[std::stoi(num)] = entry;
            }
        }
    }
    std::string menuNameBase = "Select Level";
    std::string menuName = menuNameBase;
    m_menu.addMenuItem(menuName, { menuName, MenuItemId::LEVEL_SELECT, "<back>", 0, "Main Menu" });
    int counter = 1;
    int menuCounter = 1;
    for (const auto& p : sortedPaths) {
        m_menu.addMenuItem(
            menuName,
            { menuName,
              MenuItemId::LEVEL_FILE,
              std::to_string(p.first) + " " + getLevelDescription(p.second),
              counter,
              std::nullopt,
              p.second.string() });
        ++counter;
        if (counter > 10) {
            // add to another submenu
            std::string newMenuName = menuNameBase + " (" + std::to_string(menuCounter) + ")";
            m_menu.addMenuItem(
                menuName, { menuName, MenuItemId::LEVEL_SELECT, "<more>", counter, newMenuName });
            ++menuCounter;
            menuName = newMenuName;
            m_menu.addMenuItem(
                menuName, { menuName, MenuItemId::LEVEL_SELECT, "<back>", 0, "Main Menu" });
            counter = 1;
        }
    }
    if (counter > 1) {
        m_menu.addMenuItem(
            menuName, { menuName, MenuItemId::LEVEL_SELECT, "<back>", 0, "Main Menu" });
    }
}

void GameModel::initialise(const std::string& levelFileName)
{
    // Resets the model to a state ready for a new Level
    m_livesRemaining = 1;
    m_allDynamicGameShapes.clear();

    m_shipModel.reset();
    m_shipModel
        = std::make_unique<ShipModel>(ShipModel(newGameShape(), newGameShape(), newGameShape()));

    buildBreakableExplosionShape();
    m_allDynamicGameShapes.push_back(m_breakableExplosionShape);

    // Grid lines on the background
    std::shared_ptr<GameShape> bkg(new GameShape);
    for (double n = 0; n <= 2000; n += 50) {
        ShapeLine sl1 { n, 0, n, 2000, 0, 32, 0, 255, 4 };
        bkg->addShapeLine(sl1);
        ShapeLine sl2 { 0, n, 2000, n, 0, 32, 0, 255, 4 };
        bkg->addShapeLine(sl2);
    }
    bkg->setPos(0, 0);
    bkg->setName("Background");
    bkg->setGameShapeType(GameShapeType::NEUTRAL);
    m_allDynamicGameShapes.push_back(std::move(bkg));

    m_allDynamicGameShapes.push_back(m_shipModel->shipGameShape());
    m_allDynamicGameShapes.push_back(m_shipModel->flamesGameShape());
    m_allDynamicGameShapes.push_back(m_shipModel->explosionGameShape());

    // Add all moving objects

    m_levelFileName = levelFileName;

    m_shipModel->initialise();
}

void GameModel::buildBreakableExplosionShape()
{
    using namespace helperfunctions;
    if (!m_breakableExplosionShape) {
        m_breakableExplosionShape = std::make_shared<GameShape>();
    }
    m_breakableExplosionShape->clear();
    for (int n = 0; n < 11; ++n) {
        if (rand() % 5 == 1) {
            continue;
        }
        double x1 = static_cast<double>(sine(n * 30.0) * 20 + (rand() % 10));
        double y1 = static_cast<double>(cosine(n * 30.0) * 20 + (rand() % 10));
        double x2 = static_cast<double>(sine((n + 1) * 30.0) * 20 + (rand() % 10));
        double y2 = static_cast<double>(cosine((n + 1) * 30.0) * 20 + (rand() % 10));
        m_breakableExplosionShape->addShapeLine(ShapeLine { x1, y1, x2, y2, 255, 150, 50, 255, 6 });
    }
    m_breakableExplosionShape->setName("BreakableExplosion");
    m_breakableExplosionShape->setVisible(false);
    m_breakableExplosionShape->resize(0.1);
}

std::string GameModel::levelFileName() const
{
    return m_levelFileName;
}

void GameModel::setLevelFileName(const std::string& filename)
{
    m_levelFileName = filename;
}

void GameModel::levelLoad(size_t levelNum)
{
    std::string filename = "level" + std::to_string(levelNum) + ".cfg";
    std::filesystem::path levelFile(getDataPath());
    levelFile.append(filename);
    levelLoad(levelFile.string());
}

// TODO - put this in the controller?
void GameModel::levelLoad(const std::string& filename)
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

    initialise(filename);

    std::ifstream in(filename);
    if (!in) {
        THROWUP(AmazeRuntimeException, "Failed to load Level file " + filename);
    }
    std::string currentLine;
    std::unique_ptr<GameShape> obj(new GameShape);
    // TODO - object type : ObjectType cls = ObjectType::NEUTRAL;
    while (!in.eof()) {
        getline(in, currentLine);
        std::vector<std::string> vec;
        helperfunctions::csvSplit(currentLine, '~', vec);
        // When we get here, vec is a vector of all the items on the current
        // line.
        if (vec.size() == 0) {
            continue;
        }
        char c = vec[0][0];
        switch (c) {
            case '!': // timelimit (unused), fuel (unused), ship x, ship y, ship angle,
                      // description (currently unused, but a TODO)
                m_shipModel->setShipX(stod(vec[3]));
                m_shipModel->setShipY(stod(vec[4]));
                m_shipModel->setRotation(stod(vec[5]));
                m_levelDescription = vec[6];
                break;
            case 'N': // New object, parameter 1 is type, parameter 2 appears unused
                addPreviousObject(obj);
                obj = std::unique_ptr<GameShape>(new GameShape);
                obj->setGameShapeType(GameShapeType::NEUTRAL);
                if (vec.size() > 1) {
                    if (vec[1] == "OBSTRUCTION") {
                        obj->setGameShapeType(GameShapeType::OBSTRUCTION);
                    } else if (vec[1] == "FUEL") {
                        obj->setGameShapeType(GameShapeType::FUEL);
                    } else if (vec[1] == "PRISONER") {
                        obj->setGameShapeType(GameShapeType::PRISONER);
                    } else if (vec[1] == "KEY") {
                        obj->setGameShapeType(GameShapeType::KEY);
                    } else if (vec[1] == "EXIT") {
                        obj->setGameShapeType(GameShapeType::EXIT);
                    } else if (vec[1] == "BREAKABLE") {
                        obj->setGameShapeType(GameShapeType::BREAKABLE);
                    } else if (vec[1] == "MOVING") {
                        obj->setGameShapeType(GameShapeType::MOVING);
                        if (vec.size() >= 8) {
                            obj->setName(vec[2]);
                            obj->setXDelta(std::stof(vec[3]));
                            obj->setXMaxDifference(std::stof(vec[4]));
                            obj->setYDelta(std::stof(vec[5]));
                            obj->setYMaxDifference(std::stof(vec[6]));
                            obj->setRotationDelta(std::stof(vec[7]));
                            if (vec.size() >= 9) {
                                obj->setGravity(std::stof(vec[8]));
                            }
                        }
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
                    obj->addShapeLine(sl1);
                }
                break;
            case 'P':
                if (obj != nullptr && vec.size() == 3) {
                    obj->setPos(stoi(vec[1]), stoi(vec[2]));
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
    addPreviousObject(obj);
    setLevelFileName(filename);
}

void GameModel::addPreviousObject(std::unique_ptr<marengo::amaze::GameShape>& obj)
{
    if (obj->getGameShapeType() != GameShapeType::UNINITIALISED) {
        if (obj->getGameShapeType() == GameShapeType::MOVING) {
            obj->setPosFromCentre(); // converts all lines to relative to the object's centre
        }
        m_allDynamicGameShapes.push_back(std::move(obj));
    }
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

std::tuple<GameShapeType, std::shared_ptr<GameShape>> GameModel::collisionDetect() const
{
    for (const auto& obj : m_allDynamicGameShapes) {
        if (obj == m_shipModel->shipGameShape() || obj == m_shipModel->flamesGameShape()) {
            // can't collide with itself :)
            continue;
        }
        if (obj->getGameShapeType() == GameShapeType::NEUTRAL) {
            // NEUTRAL objects don't cause collisions
            continue;
        }
        if (obj->IsActive() == false) {
            continue;
        }
        if (m_shipModel->shipGameShape()->intersectCheck(obj)) {
            return { GameShapeType::SHIP, obj };
        }
        if (m_shipModel->flamesGameShape()->isVisible()
            && m_shipModel->flamesGameShape()->intersectCheck(obj)) {
            if (obj->getGameShapeType() == GameShapeType::BREAKABLE) {
                return { GameShapeType::FLAMES, obj };
            }
        }
    }
    return { GameShapeType::NEUTRAL, nullptr };
}

void GameModel::process() // TODO more descriptive name
{
    m_scheduler.processSchedule();

    if (m_gameState == GameState::Exploding) {
        m_shipModel->setIsExploding(true);
        m_shipModel->setVisible(false);
        m_scheduler.doAfter(
            ScheduleEventName::Exploding, 40, [&]() { setGameState(GameState::Dead); });
    }
    if (m_breakableExploding) {
        m_breakableExplosionShape->setVisible(true);
        m_breakableExplosionShape->setPos(m_shipModel->x(), m_shipModel->y());
        m_breakableExplosionShape->resize(1.2);
        m_scheduler.doAfter(ScheduleEventName::BreakableExploding, 40, [&]() {
            m_breakableExploding = false;
            buildBreakableExplosionShape();
        });
    }
    m_shipModel->process(m_gameState == GameState::Exploding);

    for (const auto& shape : m_allDynamicGameShapes) {
        if (shape->getGameShapeType() == GameShapeType::MOVING
            && m_gameState == GameState::Running) {
            shape->move();
        }
        if (shape->getGameShapeType() == GameShapeType::MOVING && shape->getGravity() != 0.f) {
            double xDiff = shape->getPosX() - getShipModel()->x();
            double yDiff = shape->getPosY() - getShipModel()->y();
            double distanceSquared = xDiff * xDiff + yDiff * yDiff;
            if (distanceSquared == 0.0) {
                distanceSquared = 1.0;
            }
            double distance = std::sqrt(distanceSquared);
            if (distance < shape->getGravity() * 25) {
                auto* ship = getShipModel();
                double forceMagnitude = shape->getGravity() / distanceSquared;
                double fx = (xDiff / distance) * forceMagnitude;
                double fy = (yDiff / distance) * forceMagnitude;
                ship->setDx(ship->dX() - fx);
                ship->setDy(ship->dY() - fy);
            }
        }
    }
}

void GameModel::processDynamicObjects(std::function<void(GameShape&)> process)
{
    for (const auto& shape : m_allDynamicGameShapes) {
        process(*shape);
    }
}

unsigned int GameModel::getRotation() const
{
    return 0; // unused overridden function
}

void GameModel::savePosition()
{
    if (m_gameState == GameState::Exploding) {
        return;
    }
    unsigned x = static_cast<unsigned int>(m_shipModel->x());
    unsigned y = static_cast<unsigned int>(m_shipModel->y());
    double rot = m_shipModel->rotation();
    // The ring buffer has 200 slots, so at 100 fps the last
    // item in the buffer will be from about two seconds ago
    m_savedPositionsRingBuffer.add({ x, y, rot });
}

void GameModel::togglePause()
{
    if (m_gameState == GameState::Paused) {
        m_gameState = GameState::Running;
    } else {
        m_gameState = GameState::Paused;
    }
}

bool GameModel::gameIsPaused()
{
    return m_gameState == GameState::Paused;
}

void GameModel::restart()
{
    rebuildShip();
    m_shipModel->initialise();
    m_shipModel->setIsExploding(false);
    m_shipModel->setShipX(m_savedPositionsRingBuffer.lastItem().posX);
    m_shipModel->setShipY(m_savedPositionsRingBuffer.lastItem().posY);
    m_shipModel->setRotation(m_savedPositionsRingBuffer.lastItem().rotation);
    m_shipModel->setVisible(true);
    m_gameState = GameState::Running;
    m_shipModel->buildExplosionShape();

    std::this_thread::sleep_for(std::chrono::seconds(2));
}

GameState GameModel::getGameState()
{
    return m_gameState;
}

void GameModel::setGameState(GameState state)
{
    m_gameState = state;
}

int GameModel::lifeLost()
{
    if (m_livesRemaining > 0) {
        --m_livesRemaining;
    }
    return m_livesRemaining;
}

void GameModel::extraLife()
{
    ++m_livesRemaining;
}

void GameModel::setLives(int lives)
{
    m_livesRemaining = lives;
}

void GameModel::setBreakableExploding(bool value /* =true */)
{
    m_breakableExploding = value;
    m_breakableExplosionShape->setVisible(value);
}

void GameModel::resetMenuPosition()
{
    m_menu.resetMenuPosition();
}

std::vector<MenuItem> GameModel::getCurrentMenu()
{
    std::string currentMenuName = m_menu.getCurrentMenuName();
    auto vec = m_menu.getMenuItems(currentMenuName);
    return vec;
}

void GameModel::menuDown()
{
    m_menu.highlightNextItem();
}

void GameModel::menuUp()
{
    m_menu.highlightPreviousItem();
}

std::tuple<MenuItemId, std::optional<MenuItem>> GameModel::menuSelect()
{
    return m_menu.selectCurrentItem();
}

void GameModel::setMenu(const std::string& menuName)
{
    m_menu.getMenuItems(menuName);
}

void GameModel::rebuildShip()
{
    std::vector<std::shared_ptr<GameShape>> tmp;
    std::swap(tmp, m_allDynamicGameShapes);
    for (auto& s : tmp) {
        if (s->getName() != "Ship" && s->getName() != "Flames" && s->getName() != "Explosion") {
            m_allDynamicGameShapes.push_back(s);
        }
    }
    m_shipModel.reset();
    m_shipModel
        = std::make_unique<ShipModel>(ShipModel(newGameShape(), newGameShape(), newGameShape()));
}

} // namespace amaze
} // namespace marengo
