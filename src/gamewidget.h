#pragma once

// The main game window. It renders via OpenGL
// (paced by the display's vsync) and calls a callback once per displayed
// frame with an active QPainter. Input events are queued here and collected
// by the GameShell once per frame, so the game sees them at a predictable point
// in its frame rather than whenever Qt delivers them.

#include <QOpenGLWidget>

#include <functional>
#include <unordered_set>
#include <vector>

class QPainter;

namespace marengo {
namespace amaze {

struct GameInputEvent {
    enum class Type {
        KeyPress,
        KeyRelease,
        FocusLost,
        Close,
    };
    Type type;
    int key { 0 }; // Qt::Key
    bool shift { false };
};

class GameWidget final : public QOpenGLWidget {
public:
    explicit GameWidget(QWidget* parent = nullptr);
    void setFrameCallback(std::function<void(QPainter&)> callback);
    // Returns all input events received since the last call
    std::vector<GameInputEvent> takeInputEvents();

protected:
    void paintGL() override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    // Stop Tab moving keyboard focus, as we use it to open the menu
    bool focusNextPrevChild(bool) override
    {
        return false;
    }

private:
    std::function<void(QPainter&)> m_frameCallback;
    std::vector<GameInputEvent> m_inputEvents;
    std::unordered_set<quint32> m_keysDown; // physical keys currently held
    bool m_keyPressSeen { false }; // has the application received any key press yet?
};

} // namespace amaze
} // namespace marengo
