#include "gameshape.h"

#include <algorithm>
#include <cmath>

namespace marengo {
namespace amaze {

GameShape::GameShape() { }

bool GameShape::IsActive() const
{
    return m_isActive;
}

void GameShape::setIsActive(bool value)
{
    m_isActive = value;
}

GameShapeType GameShape::getGameShapeType() const
{
    return m_gameShapeType;
}

void GameShape::setGameShapeType(const GameShapeType& value)
{
    m_gameShapeType = value;
}

std::string GameShape::getName() const
{
    return m_name;
}

void GameShape::setName(const std::string& name)
{
    m_name = name;
}

void GameShape::move()
{
    if (!m_originalX.has_value()) {
        m_originalX = getPosX();
    }
    if (!m_originalY.has_value()) {
        m_originalY = getPosY();
    }

    if (hasPath()) {
        moveAlongPath();
        if (m_rotationDelta != 0.0f) {
            rotate(m_rotationDelta);
        }
        return;
    }

    if (getPosX() > *m_originalX + m_xMaxDifference) {
        m_xDelta = -m_xDelta;
    }
    if (getPosX() < *m_originalX - m_xMaxDifference) {
        m_xDelta = -m_xDelta;
    }
    if (getPosY() > *m_originalY + m_yMaxDifference) {
        m_yDelta = -m_yDelta;
    }
    if (getPosY() < *m_originalY - m_yMaxDifference) {
        m_yDelta = -m_yDelta;
    }
    setPos(getPosX() + m_xDelta, getPosY() + m_yDelta);

    if (m_rotationDelta != 0.0f) {
        rotate(m_rotationDelta);
    }
}

float GameShape::getRotationDelta() const
{
    return m_rotationDelta;
}

void GameShape::setRotationDelta(float value)
{
    m_rotationDelta = value;
}

float GameShape::getXDelta() const
{
    return m_xDelta;
}

float GameShape::getYDelta() const
{
    return m_yDelta;
}

void GameShape::setXDelta(float value)
{
    m_xDelta = value;
}

void GameShape::setYDelta(float value)
{
    m_yDelta = value;
}

void GameShape::setXMaxDifference(float value)
{
    m_xMaxDifference = value;
}

void GameShape::setYMaxDifference(float value)
{
    m_yMaxDifference = value;
}

float GameShape::getXMaxDifference() const
{
    return m_xMaxDifference;
}

float GameShape::getYMaxDifference() const
{
    return m_yMaxDifference;
}

float GameShape::getGravity() const
{
    return m_gravity;
}

void GameShape::setGravity(float value)
{
    m_gravity = value;
}

void GameShape::setPath(std::vector<std::pair<double, double>> offsets, double speed, bool loop)
{
    m_pathOffsets = std::move(offsets);
    m_pathSpeed = speed;
    m_pathLoops = loop;
    m_pathPoints.clear();
    m_pathDistances.clear();
    m_pathDistance = 0.0;
    m_pathForwards = true;
    if (m_pathOffsets.empty()) {
        return;
    }
    m_pathPoints.emplace_back(0.0, 0.0);
    m_pathPoints.insert(m_pathPoints.end(), m_pathOffsets.begin(), m_pathOffsets.end());
    if (loop) {
        m_pathPoints.emplace_back(0.0, 0.0);
    }
    double distance = 0.0;
    m_pathDistances.push_back(distance);
    for (std::size_t i = 1; i < m_pathPoints.size(); ++i) {
        distance += std::hypot(
            m_pathPoints[i].first - m_pathPoints[i - 1].first,
            m_pathPoints[i].second - m_pathPoints[i - 1].second);
        m_pathDistances.push_back(distance);
    }
}

bool GameShape::hasPath() const
{
    return !m_pathPoints.empty();
}

const std::vector<std::pair<double, double>>& GameShape::getPathOffsets() const
{
    return m_pathOffsets;
}

double GameShape::getPathSpeed() const
{
    return m_pathSpeed;
}

bool GameShape::getPathLoops() const
{
    return m_pathLoops;
}

void GameShape::moveAlongPath()
{
    // The position is worked out afresh from the distance along the path
    // each tick, so it never drifts off the path
    const double length = m_pathDistances.back();
    if (length > 0.0) {
        if (m_pathLoops) {
            m_pathDistance = std::fmod(m_pathDistance + m_pathSpeed, length);
        } else {
            m_pathDistance += m_pathForwards ? m_pathSpeed : -m_pathSpeed;
            // Turn round at the ends, going back by however far it overshot
            if (m_pathDistance >= length) {
                m_pathDistance = std::max(0.0, 2 * length - m_pathDistance);
                m_pathForwards = false;
            } else if (m_pathDistance <= 0.0) {
                m_pathDistance = std::min(length, -m_pathDistance);
                m_pathForwards = true;
            }
        }
    }
    const auto [dx, dy] = pathOffsetAt(m_pathDistance);
    setPos(*m_originalX + dx, *m_originalY + dy);
}

std::pair<double, double> GameShape::pathOffsetAt(double distance) const
{
    for (std::size_t i = 1; i < m_pathPoints.size(); ++i) {
        const double segmentLength = m_pathDistances[i] - m_pathDistances[i - 1];
        if (distance <= m_pathDistances[i] && segmentLength > 0.0) {
            const double t = std::clamp((distance - m_pathDistances[i - 1]) / segmentLength, 0.0, 1.0);
            const auto [x0, y0] = m_pathPoints[i - 1];
            const auto [x1, y1] = m_pathPoints[i];
            return { x0 + (x1 - x0) * t, y0 + (y1 - y0) * t };
        }
    }
    return m_pathPoints.back();
}

float GameShape::getPulsateAmount() const
{
    return m_pulsateAmount;
}

void GameShape::setPulsateAmount(float amount)
{
    m_pulsateAmount = amount;
}

} // namespace amaze
} // namespace marengo
