#include "qtgamewidget.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QPainter>

namespace marengo {
namespace amaze {

GameWidget::GameWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setWindowTitle("Amaze");
    // Continuously redraw: each completed frame schedules the next one. The
    // swap interval (vsync) paces this to the display's refresh rate.
    connect(this, &QOpenGLWidget::frameSwapped, this, qOverload<>(&QWidget::update));
}

void GameWidget::setFrameCallback(std::function<void(QPainter&)> callback)
{
    m_frameCallback = std::move(callback);
}

std::vector<GameInputEvent> GameWidget::takeInputEvents()
{
    std::vector<GameInputEvent> events;
    std::swap(events, m_inputEvents);
    return events;
}

void GameWidget::paintGL()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (m_frameCallback) {
        m_frameCallback(painter);
    }
}

void GameWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;
    }
    m_inputEvents.push_back(
        { GameInputEvent::Type::KeyPress,
          event->key(),
          event->modifiers().testFlag(Qt::ShiftModifier) });
}

void GameWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;
    }
    m_inputEvents.push_back(
        { GameInputEvent::Type::KeyRelease,
          event->key(),
          event->modifiers().testFlag(Qt::ShiftModifier) });
}

void GameWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ActivationChange && !isActiveWindow()) {
        m_inputEvents.push_back({ GameInputEvent::Type::FocusLost });
    }
    QOpenGLWidget::changeEvent(event);
}

void GameWidget::closeEvent(QCloseEvent* event)
{
    // Let the game decide when to quit; it will exit the event loop itself
    event->ignore();
    m_inputEvents.push_back({ GameInputEvent::Type::Close });
}

} // namespace amaze
} // namespace marengo
