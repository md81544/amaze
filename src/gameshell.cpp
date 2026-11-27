#include "gameshell.h"
#include "gamewidget.h"
#include "menudialog.h"

#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>

#include <cmath>
#include <exception>

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace marengo {
namespace amaze {

namespace {

// The virtual canvas always has this area (see gameshell.h)
constexpr double CANVAS_AREA = 1920.0 * 1200.0;

// Full thrust, from the keyboard or with the right stick fully pushed
constexpr float FULL_THRUST = 6.f;

// Gamepad rumble while thrusting with the right stick: none below this
// fraction of full thrust; from there it rises linearly with thrust, from
// THRUST_RUMBLE_MIN_SCALE of full strength at the threshold to full strength
// at full thrust. (Starting above zero means it's felt as soon as it starts.)
constexpr float THRUST_RUMBLE_THRESHOLD = 0.25f;
constexpr float THRUST_RUMBLE_MIN_SCALE = 0.25f;
constexpr float THRUST_RUMBLE_LOW_FREQ = 0x2000;
constexpr float THRUST_RUMBLE_HIGH_FREQ = 0xA000;
// Long enough to outlast gaps between stick events; replaced by the next one
constexpr uint32_t THRUST_RUMBLE_MS = 5000;

// Menu navigation with a stick: how far it must be pushed, and auto-repeat timing
constexpr float MENU_STICK_THRESHOLD = 0.2f;
constexpr qint64 MENU_STICK_REPEAT_DELAY_MS = 400;
constexpr qint64 MENU_STICK_REPEAT_INTERVAL_MS = 120;

// Hides/shows the cursor immediately, wherever it is. Calls must be balanced.
void setSystemCursorHidden(bool hidden)
{
#ifdef __APPLE__
    if (hidden) {
        CGDisplayHideCursor(kCGDirectMainDisplay);
    } else {
        CGDisplayShowCursor(kCGDirectMainDisplay);
    }
#else
    if (hidden) {
        QGuiApplication::setOverrideCursor(Qt::BlankCursor);
    } else {
        QGuiApplication::restoreOverrideCursor();
    }
#endif
}

} // namespace

GameShell::GameShell(
    unsigned windowWidth,
    unsigned windowHeight,
    std::optional<QRect> windowGeometry,
    bool useFullScreen,
    const MenuResults& initialMenuValues,
    const std::string& dataDir)
    : m_window(std::make_unique<GameWidget>())
    , m_menu(new MenuDialog(dataDir, initialMenuValues, m_window.get()))
    , m_useFullScreen(useFullScreen)
{
    m_clock.start();
    setStickDeadZonePercent(initialMenuValues.stickDeadZonePercent);
    setRumbleIntensityPercent(initialMenuValues.rumbleIntensityPercent);

    // Ignore saved geometry for a screen which is no longer there
    if (windowGeometry.has_value() && QGuiApplication::screenAt(windowGeometry->center())) {
        m_window->setGeometry(*windowGeometry);
    } else {
        QSize size(windowWidth, windowHeight);
        if (auto* screen = QGuiApplication::primaryScreen()) {
            size = size.boundedTo(screen->availableGeometry().size());
        }
        m_window->resize(size);
    }
    m_window->setCursor(Qt::BlankCursor);

    QObject::connect(m_menu, &QDialog::finished, m_menu, [this](int result) {
        if (result == QDialog::Accepted) {
            m_menuState = MenuType::OK;
        } else if (result == MenuDialog::ExitGame) {
            m_menuState = MenuType::Exit;
        } else {
            m_menuState = MenuType::Cancel;
        }
        m_window->activateWindow();
        m_window->setFocus();
    });

    // Never leave the cursor hidden while another application is active
    QObject::connect(
        qApp,
        &QGuiApplication::applicationStateChanged,
        m_window.get(),
        [this](Qt::ApplicationState) { updateCursor(); });
}

GameShell::~GameShell()
{
    if (m_systemCursorHidden) {
        setSystemCursorHidden(false);
    }
}

void GameShell::run(FrameFunction frameFunction)
{
    std::exception_ptr error;
    bool finished = false;
    m_window->setFrameCallback([&](QPainter& painter) {
        if (finished) {
            return;
        }
        double scale = canvasScale();
        painter.scale(scale, scale);
        QSizeF canvasSize(m_window->width() / scale, m_window->height() / scale);
        try {
            if (!frameFunction(painter, canvasSize)) {
                finished = true;
                // Note quit() can't be used: it asks windows to close first, and
                // GameWidget refuses so that the game can handle closing itself.
                QCoreApplication::exit(0);
            }
        } catch (...) {
            // Exceptions must not propagate through Qt's event loop
            error = std::current_exception();
            finished = true;
            QCoreApplication::exit(1);
        }
    });

    if (m_useFullScreen) {
        m_window->showFullScreen();
    } else {
        m_window->show();
    }
    QApplication::exec();

    m_window->setFrameCallback({});
    m_window->hide();
    if (error) {
        std::rethrow_exception(error);
    }
}

double GameShell::canvasScale() const
{
    double area = static_cast<double>(m_window->width()) * m_window->height();
    if (area <= 0.0) {
        return 1.0; // e.g. minimised
    }
    return std::sqrt(area / CANVAS_AREA);
}

int GameShell::ticks() const
{
    return static_cast<int>(m_clock.elapsed());
}

MenuType GameShell::updateMenu(MenuType menuType)
{
    // Open the menu dialog if necessary and report how it was closed
    if (m_menuState == MenuType::OK || m_menuState == MenuType::Cancel) {
        auto rc = m_menuState;
        m_menuState = MenuType::None;
        return rc;
    }
    if (m_menuState == MenuType::Exit) {
        return m_menuState;
    }
    if (menuType == MenuType::Main && !m_menu->isVisible() && !m_menuOpenPending) {
        // We're inside a paint event here, so open the dialog afterwards
        m_menuOpenPending = true;
        QTimer::singleShot(0, m_menu, [this]() {
            m_menuOpenPending = false;
            m_menuStickKey = 0;
            m_menu->show();
            m_menu->raise();
            m_menu->activateWindow();
        });
    }
    return menuType;
}

void GameShell::updateMenuLevels(const LevelProgress& progress, const std::string& currentLevelFile)
{
    m_menu->updateLevels(progress, currentLevelFile);
}

void GameShell::processMenuInput()
{
    // The dialog handles keyboard and mouse itself; we just translate gamepad
    // input into key presses for it.
    processGamepadForMenu();

    // Keys held when the menu opened will be released in the dialog, so
    // release them here
    releaseHeldKeys();

    // Discard game window input received while the menu is up (e.g. the
    // focus loss caused by the dialog opening), except a request to quit
    for (const auto& event : m_window->takeInputEvents()) {
        if (event.type == GameInputEvent::Type::Close) {
            m_controlHandlers[KeyControls::QUIT](true, 0.f);
        }
    }
}

MenuResults GameShell::menuResults() const
{
    return m_menu->results();
}

void GameShell::setMouseCursorVisible(bool value)
{
    m_cursorVisible = value;
    updateCursor();
}

void GameShell::setStickDeadZonePercent(int percent)
{
    m_gamepad.setDeadZone(percent / 100.f);
}

QRect GameShell::windowGeometry() const
{
    return m_window->normalGeometry();
}

bool GameShell::isFullScreen() const
{
    return m_window->isFullScreen();
}

void GameShell::updateCursor()
{
    auto shape = m_cursorVisible ? Qt::ArrowCursor : Qt::BlankCursor;
    if (m_window->cursor().shape() != shape) {
        m_window->setCursor(shape);
    }
    bool hideSystemCursor = !m_cursorVisible && m_window->isFullScreen()
        && QGuiApplication::applicationState() == Qt::ApplicationActive;
    if (hideSystemCursor != m_systemCursorHidden) {
        setSystemCursorHidden(hideSystemCursor);
        m_systemCursorHidden = hideSystemCursor;
    }
}

void GameShell::toggleFullScreen()
{
    // This is called during a frame, i.e. while painting, and changing the
    // window size then would pull the paint surface out from under the
    // painter. So do it once the frame is finished.
    QTimer::singleShot(0, m_window.get(), [this]() {
        if (m_window->isFullScreen()) {
            m_window->showNormal();
        } else {
            m_window->showFullScreen();
        }
        updateCursor();
    });
}

void GameShell::registerControlHandler(KeyControls key, ControlHandler controlHandler)
{
    m_controlHandlers[key] = controlHandler;
}

void GameShell::processInput(bool paused)
{
    processGamepadForGame();

    for (const auto& event : m_window->takeInputEvents()) {
        switch (event.type) {
            case GameInputEvent::Type::Close:
                m_controlHandlers[KeyControls::QUIT](true, 0.f);
                break;
            case GameInputEvent::Type::FocusLost:
                // Key releases will now go elsewhere
                releaseHeldKeys();
                // Pause when window loses focus
                if (!paused) {
                    m_controlHandlers[KeyControls::PAUSE](true, 0.f);
                }
                break;
            case GameInputEvent::Type::KeyPress:
            case GameInputEvent::Type::KeyRelease:
                processGameKeyEvent(event, paused);
                break;
        }
    }
}

void GameShell::processGameKeyEvent(const GameInputEvent& event, bool paused)
{
    // Note Qt reports the shifted character for punctuation keys, so e.g.
    // shift+comma arrives as Key_Less.
    if (event.type == GameInputEvent::Type::KeyPress) {
        switch (event.key) {
            case Qt::Key_Escape:
                if (paused) {
                    // Escape also unpauses
                    m_controlHandlers[KeyControls::PAUSE](true, 0.f);
                } else {
                    m_controlHandlers[KeyControls::MENU](true, 0.f);
                }
                break;
            case Qt::Key_Tab:
                m_controlHandlers[KeyControls::MENU](true, 0.f);
                break;
            case Qt::Key_Up:
            case Qt::Key_Space:
            case Qt::Key_Down:
                if (!paused) {
                    m_gamepadInUse = false;
                    m_shiftHeld = event.shift;
                    m_heldThrust = event.key == Qt::Key_Down ? -1 : 1;
                    applyHeldThrust();
                }
                break;
            case Qt::Key_Left:
            case Qt::Key_A:
            case Qt::Key_Comma:
            case Qt::Key_Less:
            case Qt::Key_Right:
            case Qt::Key_D:
            case Qt::Key_Period:
            case Qt::Key_Greater:
                if (!paused) {
                    m_gamepadInUse = false;
                    m_shiftHeld = event.shift;
                    bool left = event.key == Qt::Key_Left || event.key == Qt::Key_A
                        || event.key == Qt::Key_Comma || event.key == Qt::Key_Less;
                    m_heldRotation = left ? 1 : -1;
                    applyHeldRotation();
                }
                break;
            case Qt::Key_P:
                m_controlHandlers[KeyControls::PAUSE](true, 0.f);
                break;
            case Qt::Key_F:
                toggleFullScreen();
                break;
            case Qt::Key_Shift:
                // Switch any held rotate / thrust keys to fine control
                m_shiftHeld = true;
                if (!paused) {
                    applyHeldRotation();
                    applyHeldThrust();
                }
                break;
            case Qt::Key_Control:
            case Qt::Key_Meta:
            case Qt::Key_Alt:
                // Modifiers on their own do nothing (e.g. Cmd+Tab to switch
                // apps shouldn't start the game)
                break;
            default:
                m_controlHandlers[KeyControls::START](true, 0.f);
                break;
        }
    } else {
        switch (event.key) {
            case Qt::Key_Up:
            case Qt::Key_Space:
                if (m_heldThrust == 1) {
                    m_heldThrust = 0;
                }
                m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                break;
            case Qt::Key_Down:
                if (m_heldThrust == -1) {
                    m_heldThrust = 0;
                }
                m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                break;
            case Qt::Key_Left:
            case Qt::Key_A:
            case Qt::Key_Comma:
            case Qt::Key_Less:
                if (m_heldRotation == 1) {
                    m_heldRotation = 0;
                }
                m_controlHandlers[KeyControls::LEFT](false, 0.f);
                break;
            case Qt::Key_Right:
            case Qt::Key_D:
            case Qt::Key_Period:
            case Qt::Key_Greater:
                if (m_heldRotation == -1) {
                    m_heldRotation = 0;
                }
                m_controlHandlers[KeyControls::RIGHT](false, 0.f);
                break;
            case Qt::Key_Shift:
                // Switch any held rotate / thrust keys back to normal control
                m_shiftHeld = false;
                if (!paused) {
                    applyHeldRotation();
                    applyHeldThrust();
                }
                break;
            default:
                break;
        }
    }
}

void GameShell::applyHeldRotation()
{
    if (m_heldRotation == 1) {
        m_controlHandlers[KeyControls::LEFT](true, m_shiftHeld ? 0.1f : 0.75f);
    } else if (m_heldRotation == -1) {
        m_controlHandlers[KeyControls::RIGHT](true, m_shiftHeld ? -0.1f : -0.75f);
    }
}

void GameShell::applyHeldThrust()
{
    if (m_heldThrust == 1) {
        m_controlHandlers[KeyControls::ACCELERATE](true, m_shiftHeld ? 1.f : FULL_THRUST);
    } else if (m_heldThrust == -1) {
        m_controlHandlers[KeyControls::ACCELERATE](true, m_shiftHeld ? 0.4f : 1.f);
    }
}

void GameShell::releaseHeldKeys()
{
    if (m_heldRotation == 1) {
        m_controlHandlers[KeyControls::LEFT](false, 0.f);
    } else if (m_heldRotation == -1) {
        m_controlHandlers[KeyControls::RIGHT](false, 0.f);
    }
    if (m_heldThrust != 0) {
        m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
    }
    m_heldRotation = 0;
    m_heldThrust = 0;
    m_shiftHeld = false;
}

void GameShell::processGamepadForGame()
{
    for (const auto& evt : m_gamepad.getEvents()) {
        switch (evt.eventType) {
            case gamepad::EventType::Analogue:
                {
                    // Left stick for rotation
                    float r = evt.analogue.leftX;
                    if (std::abs(r) > 0.01f) { // (the dead zone is applied by Gamepad)
                        m_gamepadInUse = true;
                        m_controlHandlers[KeyControls::LR_ANALOGUE](true, -r);
                    } else {
                        if (m_gamepadInUse) {
                            m_controlHandlers[KeyControls::LR_ANALOGUE](false, 0.f);
                        }
                    }

                    // Right stick for acceleration
                    float v = evt.analogue.rightY;
                    if (v > 0.01f) {
                        m_gamepadInUse = true;
                        m_controlHandlers[KeyControls::ACCELERATE](true, v * FULL_THRUST);
                    } else {
                        if (m_gamepadInUse) {
                            m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                        }
                    }
                    updateThrustRumble(v);
                    break;
                }
            case gamepad::EventType::ButtonPressed:
                if (evt.buttonType == gamepad::ButtonType::Start) {
                    m_controlHandlers[KeyControls::MENU](true, 0.f);
                } else {
                    m_controlHandlers[KeyControls::START](true, 0.f);
                }
                break;
            default:
                break;
        }
    }
}

void GameShell::processGamepadForMenu()
{
    // Either stick, or the D-pad, moves around the dialog (arrow keys; see
    // MenuDialog::eventFilter); shoulder buttons page through the level list;
    // South (A / cross) is Enter; East (B / circle) or Start is Escape.
    qint64 now = m_clock.elapsed();
    for (const auto& evt : m_gamepad.getEvents()) {
        switch (evt.eventType) {
            case gamepad::EventType::Analogue:
                {
                    const auto& a = evt.analogue;
                    float x = std::abs(a.leftX) > std::abs(a.rightX) ? a.leftX : a.rightX;
                    float y = std::abs(a.leftY) > std::abs(a.rightY) ? a.leftY : a.rightY;
                    int key = 0;
                    if (std::abs(y) >= std::abs(x)) {
                        if (y > MENU_STICK_THRESHOLD) {
                            key = Qt::Key_Up;
                        } else if (y < -MENU_STICK_THRESHOLD) {
                            key = Qt::Key_Down;
                        }
                    } else {
                        if (x > MENU_STICK_THRESHOLD) {
                            key = Qt::Key_Right;
                        } else if (x < -MENU_STICK_THRESHOLD) {
                            key = Qt::Key_Left;
                        }
                    }
                    if (key != m_menuStickKey) {
                        if (m_menuStickKey != 0) {
                            sendMenuKey(m_menuStickKey, false);
                        }
                        if (key != 0) {
                            sendMenuKey(key, true);
                            m_menuStickRepeatAt = now + MENU_STICK_REPEAT_DELAY_MS;
                        }
                        m_menuStickKey = key;
                    }
                    break;
                }
            case gamepad::EventType::ButtonPressed:
            case gamepad::EventType::ButtonReleased:
                {
                    int key = 0;
                    switch (evt.buttonType) {
                        case gamepad::ButtonType::South:
                            key = Qt::Key_Return;
                            break;
                        case gamepad::ButtonType::East:
                        case gamepad::ButtonType::Start:
                            key = Qt::Key_Escape;
                            break;
                        case gamepad::ButtonType::DPadUp:
                            key = Qt::Key_Up;
                            break;
                        case gamepad::ButtonType::DPadDown:
                            key = Qt::Key_Down;
                            break;
                        case gamepad::ButtonType::DPadLeft:
                            key = Qt::Key_Left;
                            break;
                        case gamepad::ButtonType::DPadRight:
                            key = Qt::Key_Right;
                            break;
                        case gamepad::ButtonType::LeftShoulder:
                            key = Qt::Key_PageUp;
                            break;
                        case gamepad::ButtonType::RightShoulder:
                            key = Qt::Key_PageDown;
                            break;
                        default:
                            break;
                    }
                    if (key != 0) {
                        sendMenuKey(key, evt.eventType == gamepad::EventType::ButtonPressed);
                    }
                    break;
                }
            default:
                break;
        }
    }
    // Auto-repeat while a stick is held over
    if (m_menuStickKey != 0 && now >= m_menuStickRepeatAt) {
        sendMenuKey(m_menuStickKey, true);
        m_menuStickRepeatAt = now + MENU_STICK_REPEAT_INTERVAL_MS;
    }
}

void GameShell::sendMenuKey(int key, bool pressed)
{
    if (!m_menu->isVisible()) {
        return;
    }
    QWidget* target = QApplication::focusWidget();
    if (target == nullptr || (target != m_menu && !m_menu->isAncestorOf(target))) {
        target = m_menu;
    }
    QCoreApplication::postEvent(
        target, new QKeyEvent(pressed ? QEvent::KeyPress : QEvent::KeyRelease, key, Qt::NoModifier));
}

void GameShell::updateThrustRumble(float thrust)
{
    if (thrust >= THRUST_RUMBLE_THRESHOLD) {
        float t = (thrust - THRUST_RUMBLE_THRESHOLD) / (1.f - THRUST_RUMBLE_THRESHOLD);
        float scale = THRUST_RUMBLE_MIN_SCALE + (1.f - THRUST_RUMBLE_MIN_SCALE) * std::min(t, 1.f);
        m_gamepad.rumble(static_cast<uint16_t>(THRUST_RUMBLE_LOW_FREQ * scale),
            static_cast<uint16_t>(THRUST_RUMBLE_HIGH_FREQ * scale),
            THRUST_RUMBLE_MS);
        m_thrustRumbling = true;
    } else if (m_thrustRumbling) {
        // Only stop a rumble this started, not e.g. a scrape's
        m_gamepad.rumble(0, 0, 0);
        m_thrustRumbling = false;
    }
}

void GameShell::rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs)
{
    m_gamepad.rumble(lowFreqIntensity, highFreqIntensity, durationMs);
    // Any rumble replaces the thrust one
    m_thrustRumbling = false;
}

void GameShell::setRumbleIntensityPercent(int percent)
{
    m_gamepad.setRumbleIntensity(percent / 100.f);
}

} // namespace amaze
} // namespace marengo
