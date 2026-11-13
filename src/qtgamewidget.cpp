#include "qtgamewidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QPainter>

namespace marengo {
namespace amaze {

namespace {

// Identifies the physical key, which (unlike QKeyEvent::key()) is unaffected
// by modifiers, e.g. shift+comma reports Key_Less
quint32 physicalKey(const QKeyEvent* event)
{
#ifdef Q_OS_MACOS
    quint32 id = event->nativeVirtualKey();
#else
    quint32 id = event->nativeScanCode();
#endif
    return id != 0 ? id : static_cast<quint32>(event->key());
}

} // namespace

GameWidget::GameWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setWindowTitle("Amaze");
    // Continuously redraw: each completed frame schedules the next one. The
    // swap interval (vsync) paces this to the display's refresh rate.
    connect(this, &QOpenGLWidget::frameSwapped, this, qOverload<>(&QWidget::update));
    // Watch for the application's first key press (see keyReleaseEvent())
    qApp->installEventFilter(this);
}

bool GameWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        m_keyPressSeen = true;
        qApp->removeEventFilter(this);
    }
    return QOpenGLWidget::eventFilter(watched, event);
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
    // We only want the initial press of a key, not auto-repeats. We can't rely
    // on isAutoRepeat() for this: on macOS the first key press after startup
    // can arrive flagged as an auto-repeat. So we track which keys are down.
    if (!m_keysDown.insert(physicalKey(event)).second) {
        return; // already down, so this is an auto-repeat
    }
    m_inputEvents.push_back(
        { GameInputEvent::Type::KeyPress,
          event->key(),
          event->modifiers().testFlag(Qt::ShiftModifier) });
}

void GameWidget::keyReleaseEvent(QKeyEvent* event)
{
    // Some platforms send auto-repeats as release/press pairs
    if (event->isAutoRepeat()) {
        return;
    }
    bool wasDown = m_keysDown.erase(physicalKey(event)) != 0;
    if (!wasDown && !m_keyPressSeen) {
        // On macOS the application's first key press can go missing, so that
        // a quick tap arrives as just a release. (If the key's held, the
        // auto-repeats arrive, which keyPressEvent() handles.) Supply the
        // missing press. Only for the first key, though: otherwise a release
        // without a press is normal, e.g. if the press went to the menu dialog.
        m_keyPressSeen = true;
        qApp->removeEventFilter(this);
        m_inputEvents.push_back(
            { GameInputEvent::Type::KeyPress,
              event->key(),
              event->modifiers().testFlag(Qt::ShiftModifier) });
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
        // Key releases now go elsewhere (e.g. to the menu dialog), so forget
        // which keys were down rather than wait for releases we won't get
        m_keysDown.clear();
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
