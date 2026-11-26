#include "gamemodel.h"
#include "levelfile.h"
#include "log.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>

namespace marengo {
namespace amaze {

namespace {

// Proportion of the ship's speed towards an obstacle which it bounces back with
constexpr double BOUNCE_RESTITUTION = 0.5;
// How hard the ship is to spin, relative to its mass. Higher means a hit off
// centre (e.g. on a wing) spins it less, and deflects it more.
constexpr double SHIP_MOMENT_OF_INERTIA = 500.0;
// Friction slowing the ship along an obstacle on impact. The speed lost is
// this proportion of the impact's impulse, so a graze barely slows the ship
// but a hard hit does.
constexpr double BOUNCE_FRICTION = 0.2;
// If the ship is still touching an obstacle after a bounce (e.g. it rotated
// into it, or a moving object moved into it), it's pushed clear in steps of
// this size, up to a limit
constexpr double PUSH_OUT_STEP = 0.25;
constexpr int MAX_PUSH_OUT_STEPS = 40;

// Impacts (see bounceShip()) slower than this don't weaken the shield, e.g.
// when the ship is thrust against a wall
constexpr double SHIELD_DAMAGE_MIN_SPEED = 0.02;
// The slowest impacts which do weaken the shield take this much off it, up
// to all of it for impacts at least this fast
constexpr double SHIELD_DAMAGE_MIN = 0.1;
constexpr double SHIELD_DAMAGE_MAX_SPEED = 0.3;
// Impacts within this many ticks of each other count as one, e.g. the ship
// bouncing a few times as it comes to rest against a wall. Only damage more
// than the earlier impact's is taken.
constexpr int SHIELD_IMPACT_TICKS = 60;

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
    m_shield = 1.0;
    m_recentShieldDamage = 0.0;
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
    std::string filename = "level" + std::to_string(levelNum) + ".lvl";
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
        if (obj->getGameShapeType() == GameShapeType::NEUTRAL
            || obj->getGameShapeType() == GameShapeType::UNINITIALISED) {
            // NEUTRAL objects don't cause collisions, nor do the explosions
            // (which are UNINITIALISED). Explosion shapes stay where they
            // were last used, even when invisible.
            continue;
        }
        if (obj->IsActive() == false) {
            continue;
        }
        if (obj->getGameShapeType() == GameShapeType::FUEL && m_shield >= 1.0) {
            // Fuel is only collected when the shield needs recharging, and
            // otherwise is passed over (so it doesn't hide anything else the
            // ship is touching)
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

std::optional<Shape::Contact> GameModel::shipContact() const
{
    QPointF pointSum;
    QPointF normalSum;
    int touching = 0;
    for (const auto& obj : m_allDynamicGameShapes) {
        if (!obj->IsActive() || !isObstacle(obj->getGameShapeType())) {
            continue;
        }
        if (auto contact = m_shipModel->shipGameShape()->contact(obj)) {
            pointSum += contact->point;
            normalSum += contact->normal;
            ++touching;
        }
    }
    double length = std::hypot(normalSum.x(), normalSum.y());
    if (touching == 0 || length < 1e-9) {
        return std::nullopt;
    }
    return Shape::Contact { pointSum / touching, normalSum / length };
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
        auto contact = shipContact();
        if (!contact) {
            break;
        }
        ship.setShipX(ship.x() + contact->normal.x() * PUSH_OUT_STEP);
        ship.setShipY(ship.y() + contact->normal.y() * PUSH_OUT_STEP);
    }
    return !shipTouchesObstacle();
}

Impact GameModel::bounceShip()
{
    auto& ship = *m_shipModel;
    auto contact = shipContact();
    QPointF move = ship.lastMove();
    std::optional<QPointF> normal;
    double impactSpeed = 0.0;

    if (contact) {
        normal = contact->normal;
        const QPointF& n = contact->normal;
        // Where the ship was hit, relative to its centre
        QPointF r = contact->point - QPointF(ship.x(), ship.y());
        // Spin in radians per tick, in the same sense as Shape::rotate() (the
        // ship's shape is rotated the opposite way to its rotation)
        double spin = -ship.spin() * std::numbers::pi / 180.0;
        // Note the ship moves by -(dX, dY) each tick
        QPointF motion(-ship.dX(), -ship.dY());
        // How fast the part of the ship which was hit is moving, including
        // from spin
        QPointF pointMotion = motion + QPointF(-r.y(), r.x()) * spin;
        impactSpeed = -QPointF::dotProduct(pointMotion, n);
        if (impactSpeed > 0.0) {
            // The impulse from the obstacle bounces the hit part of the ship
            // back off it. A hit away from the centre (e.g. on a wing) puts
            // some of that into spinning the ship, and less into deflecting it.
            double leverage = r.x() * n.y() - r.y() * n.x();
            double impulse = (1.0 + BOUNCE_RESTITUTION) * impactSpeed
                / (1.0 + leverage * leverage / SHIP_MOMENT_OF_INERTIA);
            motion += n * impulse;
            spin += leverage * impulse / SHIP_MOMENT_OF_INERTIA;

            // Friction slows the motion along the obstacle
            QPointF towards = n * QPointF::dotProduct(motion, n);
            QPointF along = motion - towards;
            double alongSpeed = std::hypot(along.x(), along.y());
            if (alongSpeed > 0.0) {
                along *= std::max(alongSpeed - BOUNCE_FRICTION * impulse, 0.0) / alongSpeed;
            }
            motion = along + towards;
            ship.setDx(-motion.x());
            ship.setDy(-motion.y());
            ship.setSpin(-spin * 180.0 / std::numbers::pi);
        }
    }

    // Go back to where the ship was before this tick's move. Unless it rotated
    // into something (or a moving object moved into it) it's then clear.
    ship.revertPosition();
    bool pushed = shipTouchesObstacle();

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

    return Impact { std::max(impactSpeed, 0.0), pushed };
}

double GameModel::shield() const
{
    return m_shield;
}

void GameModel::shieldImpact(const Impact& impact)
{
    double impactSpeed = impact.speed;
    if (impact.pushed) {
        impactSpeed = std::max(impactSpeed, SHIELD_DAMAGE_MIN_SPEED);
    }
    if (impactSpeed < SHIELD_DAMAGE_MIN_SPEED) {
        return;
    }
    double severity = std::min(
        (impactSpeed - SHIELD_DAMAGE_MIN_SPEED)
            / (SHIELD_DAMAGE_MAX_SPEED - SHIELD_DAMAGE_MIN_SPEED),
        1.0);
    double damage = SHIELD_DAMAGE_MIN + (1.0 - SHIELD_DAMAGE_MIN) * severity;
    if (m_ticksSinceShieldDamage >= SHIELD_IMPACT_TICKS) {
        m_recentShieldDamage = 0.0;
    }
    if (damage > m_recentShieldDamage) {
        m_shield = std::max(m_shield - (damage - m_recentShieldDamage), 0.0);
        m_recentShieldDamage = damage;
    }
    m_ticksSinceShieldDamage = 0;
    m_shipModel->setShieldLevel(m_shield);
}

void GameModel::rechargeShield(double amount)
{
    m_shield = std::min(m_shield + amount, 1.0);
    m_shipModel->setShieldLevel(m_shield);
}

void GameModel::tick()
{
    m_scheduler.processSchedule();
    ++m_ticksSinceShieldDamage;

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

GameState GameModel::getGameState()
{
    return m_gameState;
}

void GameModel::setGameState(GameState state)
{
    m_gameState = state;
}

void GameModel::setBreakableExploding(bool value /* =true */)
{
    m_breakableExploding = value;
    m_breakableExplosionShape->setVisible(value);
}

} // namespace amaze
} // namespace marengo
