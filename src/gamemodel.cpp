#include "gamemodel.h"
#include "levelfile.h"
#include "log.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace marengo {
namespace amaze {

namespace {

// Proportion of the ship's speed towards an obstacle which it bounces back with
constexpr double BOUNCE_RESTITUTION = 0.5;
// Friction slowing the ship along an obstacle on impact. The speed lost is
// this proportion of the change in speed towards the obstacle, so a graze
// barely slows the ship but a hard hit does.
constexpr double BOUNCE_FRICTION = 0.2;
// If the ship is still touching an obstacle after a bounce (e.g. it rotated
// into it, or a moving object moved into it), it's pushed clear in steps of
// this size, up to a limit
constexpr double PUSH_OUT_STEP = 0.25;
constexpr int MAX_PUSH_OUT_STEPS = 40;

bool isObstacle(GameShapeType type)
{
    return type == GameShapeType::OBSTRUCTION || type == GameShapeType::MOVING
        || type == GameShapeType::BREAKABLE;
}

} // namespace

GameModel::GameModel(const std::string& dataPath)
    : m_dataPath(dataPath)
{
    m_shipModel
        = std::make_unique<ShipModel>(ShipModel(newGameShape(), newGameShape(), newGameShape()));
}

void GameModel::initialise(const std::string& levelFileName)
{
    // Resets the model to a state ready for a new Level
    m_livesRemaining = 1;
    m_levelDescription.clear();
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
    using namespace utils;
    if (!m_breakableExplosionShape) {
        m_breakableExplosionShape = std::make_shared<GameShape>();
    }
    m_breakableExplosionShape->clear();
    for (int n = 0; n < 11; ++n) {
        if (utils::rnd(5) == 1) {
            continue;
        }
        double x1 = static_cast<double>(sine(n * 30.0) * 20 + (utils::rnd(10)));
        double y1 = static_cast<double>(cosine(n * 30.0) * 20 + (utils::rnd(10)));
        double x2 = static_cast<double>(sine((n + 1) * 30.0) * 20 + (utils::rnd(10)));
        double y2 = static_cast<double>(cosine((n + 1) * 30.0) * 20 + (utils::rnd(10)));
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

void GameModel::levelLoad(const std::string& filename)
{
    mgo::Log::debug(std::format("Loading from '{}'", filename));
    LevelData level = readLevelFile(filename);
    initialise(filename);
    m_shipModel->setShipX(level.shipX);
    m_shipModel->setShipY(level.shipY);
    m_shipModel->setRotation(level.shipRotation);
    m_levelDescription = level.description;
    for (auto& shape : level.shapes) {
        m_allDynamicGameShapes.push_back(std::move(shape));
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

ShipModel& GameModel::shipModel()
{
    return *m_shipModel;
}

const ShipModel& GameModel::shipModel() const
{
    return *m_shipModel;
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

std::optional<QPointF> GameModel::shipContactNormal() const
{
    QPointF sum;
    bool touching = false;
    for (const auto& obj : m_allDynamicGameShapes) {
        if (!obj->IsActive() || !isObstacle(obj->getGameShapeType())) {
            continue;
        }
        if (auto normal = m_shipModel->shipGameShape()->contactNormal(obj)) {
            sum += *normal;
            touching = true;
        }
    }
    double length = std::hypot(sum.x(), sum.y());
    if (!touching || length < 1e-9) {
        return std::nullopt;
    }
    return sum / length;
}

bool GameModel::shipTouchesObstacle() const
{
    for (const auto& obj : m_allDynamicGameShapes) {
        if (obj->IsActive() && isObstacle(obj->getGameShapeType())
            && m_shipModel->shipGameShape()->intersectCheck(obj)) {
            return true;
        }
    }
    return false;
}

bool GameModel::pushShipClear()
{
    auto& ship = *m_shipModel;
    for (int n = 0; n < MAX_PUSH_OUT_STEPS; ++n) {
        auto direction = shipContactNormal();
        if (!direction) {
            break;
        }
        ship.setShipX(ship.x() + direction->x() * PUSH_OUT_STEP);
        ship.setShipY(ship.y() + direction->y() * PUSH_OUT_STEP);
    }
    return !shipTouchesObstacle();
}

double GameModel::bounceShip()
{
    auto& ship = *m_shipModel;
    auto normal = shipContactNormal();
    QPointF move = ship.lastMove();

    // Go back to where the ship was before this tick's move. Unless it rotated
    // into something (or a moving object moved into it) it's then clear.
    ship.revertPosition();

    double impactSpeed = 0.0;
    if (normal) {
        // Note the ship moves by -(dX, dY) each tick
        QPointF motion(-ship.dX(), -ship.dY());
        impactSpeed = -QPointF::dotProduct(motion, *normal);
        if (impactSpeed > 0.0) {
            // Reverse the part of the motion towards the obstacle, and slow the
            // part along it by friction
            QPointF towards = *normal * -impactSpeed;
            QPointF along = motion - towards;
            double alongSpeed = std::hypot(along.x(), along.y());
            if (alongSpeed > 0.0) {
                double friction = BOUNCE_FRICTION * (1.0 + BOUNCE_RESTITUTION) * impactSpeed;
                along *= std::max(alongSpeed - friction, 0.0) / alongSpeed;
            }
            motion = along - towards * BOUNCE_RESTITUTION;
            ship.setDx(-motion.x());
            ship.setDy(-motion.y());
        }
    }

    // Rotating is never blocked by an obstacle, as that can leave the ship
    // unable to turn away from it; the ship is pushed clear instead. Only if
    // that isn't possible (e.g. somewhere too narrow to turn) is the rotation
    // undone.
    if (!pushShipClear()) {
        ship.revertPosition();
        ship.revertRotation();
        pushShipClear();
    }

    // Still make this tick's move, apart from any part of it into the
    // obstacle, so the ship keeps going when grazing it rather than pausing
    if (normal) {
        QPointF slide = move - *normal * std::min(QPointF::dotProduct(move, *normal), 0.0);
        double clearX = ship.x();
        double clearY = ship.y();
        ship.setShipX(clearX + slide.x());
        ship.setShipY(clearY + slide.y());
        if (!pushShipClear()) {
            ship.setShipX(clearX);
            ship.setShipY(clearY);
        }
    }

    return std::max(impactSpeed, 0.0);
}

void GameModel::tick()
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
    m_shipModel->tick(m_gameState == GameState::Exploding);

    for (const auto& shape : m_allDynamicGameShapes) {
        if (shape->getGameShapeType() == GameShapeType::MOVING
            && m_gameState == GameState::Running) {
            shape->move();
        }
        if (shape->getGameShapeType() == GameShapeType::MOVING && shape->getGravity() != 0.f) {

            if (shape->getScale() >= 1.01f) {
                shape->setPulsateAmount(-0.001f);
            }
            if (shape->getScale() <= 0.99) {
                shape->setPulsateAmount(0.001f);
            }
            shape->resize(shape->getScale() + shape->getPulsateAmount());

            double xDiff = shape->getPosX() - shipModel().x();
            double yDiff = shape->getPosY() - shipModel().y();
            double distanceSquared = xDiff * xDiff + yDiff * yDiff;
            if (distanceSquared == 0.0) {
                distanceSquared = 1.0;
            }
            double distance = std::sqrt(distanceSquared);
            if (distance < shape->getGravity() * 25) {
                auto& ship = shipModel();
                double forceMagnitude = shape->getGravity() / distanceSquared;
                double fx = (xDiff / distance) * forceMagnitude;
                double fy = (yDiff / distance) * forceMagnitude;
                ship.setDx(ship.dX() - fx);
                ship.setDy(ship.dY() - fy);
            }
        }
    }
}

const std::vector<std::shared_ptr<GameShape>>& GameModel::getAllDynamicObjects() const
{
    return m_allDynamicGameShapes;
}

void GameModel::savePosition()
{
    // Don't record the crash site, otherwise a restart would put the ship
    // straight back where it died
    if (m_gameState == GameState::Exploding || m_gameState == GameState::Dead) {
        return;
    }
    unsigned x = static_cast<unsigned int>(m_shipModel->x());
    unsigned y = static_cast<unsigned int>(m_shipModel->y());
    double rot = m_shipModel->rotation();
    // The ring buffer has 200 slots, so at 120 ticks per second the last
    // item in the buffer will be from about 1.7 seconds ago
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
