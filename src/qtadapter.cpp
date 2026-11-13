#include "qtadapter.h"
#include "exceptions.h"
#include "gamepad.h"
#include "log.h"
#include "qtgamewidget.h"
#include "qtmenudialog.h"

#include <QApplication>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QSoundEffect>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <exception>
#include <format>

namespace marengo {
namespace amaze {

namespace {

// The virtual canvas always has this area (see qtadapter.h)
constexpr double CANVAS_AREA = 1920.0 * 1200.0;

// Menu navigation with a stick: how far it must be pushed, and auto-repeat timing
constexpr float MENU_STICK_THRESHOLD = 0.2f;
constexpr qint64 MENU_STICK_REPEAT_DELAY_MS = 400;
constexpr qint64 MENU_STICK_REPEAT_INTERVAL_MS = 120;

void checkFileExists(const std::string& filename)
{
    if (!QFileInfo::exists(QString::fromStdString(filename))) {
        THROWUP(AmazeRuntimeException, std::format("File not found ({})", filename));
    }
}

} // namespace

QtAdapter::QtAdapter(
    unsigned windowWidth,
    unsigned windowHeight,
    bool useFullScreen,
    const std::string& dataDir)
    : m_window(std::make_unique<GameWidget>())
    , m_menu(new MenuDialog(dataDir, m_window.get()))
    , m_useFullScreen(useFullScreen)
{
    m_clock.start();

    QSize size(windowWidth, windowHeight);
    if (auto* screen = QGuiApplication::primaryScreen()) {
        size = size.boundedTo(screen->availableGeometry().size());
    }
    m_window->resize(size);
    m_window->setCursor(Qt::BlankCursor);

    int fontId = QFontDatabase::addApplicationFont(
        QString::fromStdString(dataDir + "/Oxanium-SemiBold.ttf"));
    if (fontId < 0) {
        THROWUP(AmazeRuntimeException, "Font file load error");
    }
    m_font = QFont(QFontDatabase::applicationFontFamilies(fontId).at(0));

    m_music.setAudioOutput(&m_musicOutput);
    QObject::connect(
        &m_music,
        &QMediaPlayer::errorOccurred,
        &m_music,
        [](QMediaPlayer::Error, const QString& message) {
            mgo::Log::error(std::format("Music playback error: {}", message.toStdString()));
        });

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
}

QtAdapter::~QtAdapter() = default;

void QtAdapter::run(std::function<bool()> frameFunction)
{
    std::exception_ptr error;
    bool finished = false;
    m_window->setFrameCallback([&](QPainter& painter) {
        if (finished) {
            return;
        }
        m_painter = &painter;
        painter.scale(canvasScale(), canvasScale());
        try {
            if (!frameFunction()) {
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
        m_painter = nullptr;
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

double QtAdapter::canvasScale() const
{
    double area = static_cast<double>(m_window->width()) * m_window->height();
    if (area <= 0.0) {
        return 1.0; // e.g. minimised
    }
    return std::sqrt(area / CANVAS_AREA);
}

void QtAdapter::setVsync(bool)
{
    // Nothing to do: Qt's default surface format already syncs to the display,
    // and frames are paced by GameWidget redrawing after each buffer swap.
}

void QtAdapter::cls()
{
    // Fill slightly beyond the canvas to avoid any rounding gap at the edges
    m_painter->fillRect(QRectF(0, 0, getWindowWidth() + 1, getWindowHeight() + 1), Qt::black);
}

void QtAdapter::redraw()
{
    // Nothing to do: Qt presents the frame once GameWidget::paintGL() returns
}

void QtAdapter::drawLine(
    float xFrom,
    float yFrom,
    float xTo,
    float yTo,
    float width,
    int r,
    int g,
    int b)
{
    m_painter->setPen(QPen(QColor(r, g, b), width, Qt::SolidLine, Qt::RoundCap));
    m_painter->drawLine(QLineF(xFrom, yFrom, xTo, yTo));
}

int QtAdapter::getWindowWidth() const
{
    return static_cast<int>(std::lround(m_window->width() / canvasScale()));
}

int QtAdapter::getWindowHeight() const
{
    return static_cast<int>(std::lround(m_window->height() / canvasScale()));
}

int QtAdapter::getTicks() const
{
    return static_cast<int>(m_clock.elapsed());
}

void QtAdapter::drawStatusBar()
{
    m_painter->fillRect(QRectF(0, 0, getWindowWidth() + 1, 75), Qt::black);
}

void QtAdapter::drawText(const Text& text)
{
    QFont font(m_font);
    font.setPixelSize(static_cast<int>(text.characterSize));
    QFontMetricsF metrics(font);
    QString string = QString::fromStdString(text.text);

    // x and y position are optional, with no value signifying centred in that dimension
    double x = text.positionX.has_value()
        ? text.positionX.value()
        : (getWindowWidth() - metrics.horizontalAdvance(string)) / 2.0;
    double baseline = text.positionY.has_value()
        ? text.positionY.value() + metrics.ascent()
        : (getWindowHeight() + metrics.ascent() - metrics.descent()) / 2.0;

    m_painter->setFont(font);
    m_painter->setPen(QColor(text.r, text.g, text.b));
    m_painter->drawText(QPointF(x, baseline), string);
}

MenuType QtAdapter::menuDraw(MenuType menuType)
{
    // The menu is a separate dialog window, so there's nothing to draw here;
    // just open it if necessary and report how it was closed.
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

void QtAdapter::menuProcessEvents()
{
    // The dialog handles keyboard and mouse itself; we just translate gamepad
    // input into key presses for it.
    processGamepadForMenu();

    // Discard game window input received while the menu is up (e.g. the
    // focus loss caused by the dialog opening), except a request to quit
    for (const auto& event : m_window->takeInputEvents()) {
        if (event.type == GameInputEvent::Type::Close) {
            m_controlHandlers[KeyControls::QUIT](true, 0.f);
        }
    }
}

MenuResults QtAdapter::menuGetResults()
{
    return m_menu->results();
}

void QtAdapter::setMouseCursorVisible(bool value)
{
    auto shape = value ? Qt::ArrowCursor : Qt::BlankCursor;
    if (m_window->cursor().shape() != shape) {
        m_window->setCursor(shape);
    }
}

void QtAdapter::registerControlHandler(
    KeyControls key,
    std::function<void(const bool, const float)> controlHandler)
{
    m_controlHandlers[key] = controlHandler;
}

void QtAdapter::processInput(bool paused)
{
    // This is called once per frame, unless a menu is active.
    processGamepadForGame();

    for (const auto& event : m_window->takeInputEvents()) {
        switch (event.type) {
            case GameInputEvent::Type::Close:
                m_controlHandlers[KeyControls::QUIT](true, 0.f);
                break;
            case GameInputEvent::Type::FocusLost:
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

void QtAdapter::processGameKeyEvent(const GameInputEvent& event, bool paused)
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
                if (!paused) {
                    m_gamepadInUse = false;
                    m_controlHandlers[KeyControls::ACCELERATE](true, event.shift ? 1.f : 6.f);
                }
                break;
            case Qt::Key_Down:
                if (!paused) {
                    m_gamepadInUse = false;
                    m_controlHandlers[KeyControls::ACCELERATE](true, event.shift ? 0.4f : 1.f);
                }
                break;
            case Qt::Key_Left:
            case Qt::Key_A:
            case Qt::Key_Comma:
            case Qt::Key_Less:
                if (!paused) {
                    m_gamepadInUse = false;
                    m_controlHandlers[KeyControls::LEFT](true, event.shift ? 0.1f : 0.75f);
                }
                break;
            case Qt::Key_Right:
            case Qt::Key_D:
            case Qt::Key_Period:
            case Qt::Key_Greater:
                if (!paused) {
                    m_gamepadInUse = false;
                    m_controlHandlers[KeyControls::RIGHT](true, event.shift ? -0.1f : -0.75f);
                }
                break;
            case Qt::Key_P:
                m_controlHandlers[KeyControls::PAUSE](true, 0.f);
                break;
            default:
                break;
        }
    } else {
        switch (event.key) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Space:
                m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                break;
            case Qt::Key_Left:
            case Qt::Key_A:
            case Qt::Key_Comma:
            case Qt::Key_Less:
                m_controlHandlers[KeyControls::LEFT](false, 0.f);
                break;
            case Qt::Key_Right:
            case Qt::Key_D:
            case Qt::Key_Period:
            case Qt::Key_Greater:
                m_controlHandlers[KeyControls::RIGHT](false, 0.f);
                break;
            default:
                break;
        }
    }
}

void QtAdapter::processGamepadForGame()
{
    for (const auto& evt : m_gamepad.getEvents()) {
        switch (evt.eventType) {
            case gamepad::EventType::Analogue:
                {
                    // Left stick for rotation
                    float r = evt.analogue.leftX;
                    if (std::abs(r) > 0.01f) { // TODO configurable deadzone
                        m_gamepadInUse = true;
                        m_controlHandlers[KeyControls::LR_ANALOGUE](true, -r);
                    } else {
                        if (m_gamepadInUse) {
                            m_controlHandlers[KeyControls::LR_ANALOGUE](false, 0.f);
                        }
                    }

                    // Right stick for acceleration
                    float v = evt.analogue.rightY;
                    if (v > 0.01f) { // TODO configurable deadzone
                        m_gamepadInUse = true;
                        m_controlHandlers[KeyControls::ACCELERATE](true, v * 15.f);
                        if (v > 0.1f) {
                            m_gamepad.rumble(
                                static_cast<uint16_t>(static_cast<float>(0x2000) * v),
                                static_cast<uint16_t>(static_cast<float>(0xA000) * v),
                                5000);
                        }
                    } else {
                        if (m_gamepadInUse) {
                            m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                        }
                        m_gamepad.rumble(0, 0, 0);
                    }
                    break;
                }
            case gamepad::EventType::ButtonPressed:
                if (evt.buttonType == gamepad::ButtonType::Start) {
                    m_controlHandlers[KeyControls::MENU](true, 0.f);
                }
                break;
            default:
                break;
        }
    }
}

void QtAdapter::processGamepadForMenu()
{
    // Either stick, or the D-pad, moves around the dialog (arrow keys);
    // shoulder buttons move between controls (tab); South (A / cross) is
    // Enter; East (B / circle) or Start is Escape.
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
                            key = Qt::Key_Backtab;
                            break;
                        case gamepad::ButtonType::RightShoulder:
                            key = Qt::Key_Tab;
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

void QtAdapter::sendMenuKey(int key, bool pressed)
{
    if (!m_menu->isVisible()) {
        return;
    }
    QWidget* target = QApplication::focusWidget();
    if (target == nullptr || (target != m_menu && !m_menu->isAncestorOf(target))) {
        target = m_menu;
    }
    auto modifiers = key == Qt::Key_Backtab ? Qt::ShiftModifier : Qt::NoModifier;
    QCoreApplication::postEvent(
        target, new QKeyEvent(pressed ? QEvent::KeyPress : QEvent::KeyRelease, key, modifiers));
}

QSoundEffect& QtAdapter::sound(const std::string& key)
{
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) {
        THROWUP(AmazeRuntimeException, std::format("Sound not loaded ({})", key));
    }
    return *it->second;
}

void QtAdapter::soundLoad(const std::string& key, const std::string& filename)
{
    checkFileExists(filename);
    auto effect = std::make_unique<QSoundEffect>();
    effect->setSource(QUrl::fromLocalFile(QString::fromStdString(filename)));
    m_sounds[key] = std::move(effect);
}

void QtAdapter::soundPlay(const std::string& key)
{
    auto& s = sound(key);
    s.setVolume(1.f);
    if (!s.isPlaying()) {
        s.setLoopCount(1);
        s.play();
    }
}

void QtAdapter::soundLoop(const std::string& key, float volume)
{
    // volume is a percentage
    auto& s = sound(key);
    s.setVolume(std::clamp(volume, 0.f, 100.f) / 100.f);
    if (!s.isPlaying()) {
        s.setLoopCount(QSoundEffect::Infinite);
        s.play();
    }
}

void QtAdapter::soundFade(const std::string& key, const int /* msecs */)
{
    // Reduce the volume a bit on each call
    auto& s = sound(key);
    s.setVolume(std::max(s.volume() - 0.03f, 0.f));
}

void QtAdapter::musicLoad(const std::string& filename)
{
    checkFileExists(filename);
    m_music.setSource(QUrl::fromLocalFile(QString::fromStdString(filename)));
}

void QtAdapter::musicPlayLoop()
{
    m_music.setLoops(QMediaPlayer::Infinite);
    m_musicOutput.setVolume(1.f);
    m_music.play();
}

void QtAdapter::setMusicVolumePercent(float vol)
{
    m_musicOutput.setVolume(std::clamp(vol, 0.f, 100.f) / 100.f);
}

void QtAdapter::rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs)
{
    m_gamepad.rumble(lowFreqIntensity, highFreqIntensity, durationMs);
}

float QtAdapter::getScalingFactor()
{
    // The canvas always has the area of a 1920x1200 screen, which the SFML
    // adapter's scaling (screen height / 1080) puts at:
    return 1200.f / 1080.f;
}

} // namespace amaze
} // namespace marengo
