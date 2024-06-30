#include "sfmladapter.h"
#include "exceptions.h"
#include "helperfunctions.h"
#include "log.h"

#include <chrono>
#include <thread>

namespace marengo {
namespace amaze {

SfmlAdapter::SfmlAdapter(int screenWidth, int screenHeight, bool useFullScreen)
    : m_window(sf::VideoMode(screenWidth, screenHeight),
          "Amaze",
          useFullScreen ? sf::Style::Fullscreen : sf::Style::Default)
    , m_screenHeight(screenHeight)
    , m_screenWidth(screenWidth)
{
    m_window.setMouseCursorVisible(false);
}

SfmlAdapter::~SfmlAdapter() { }

int SfmlAdapter::getWindowHeight() const
{
    return m_screenHeight;
}

int SfmlAdapter::getTicks() const
{
    sf::Clock clock;
    return clock.getElapsedTime().asMilliseconds();
}

int SfmlAdapter::getWindoWidth() const
{
    return m_screenWidth;
}

void SfmlAdapter::cls()
{
    m_window.clear();
}

void SfmlAdapter::redraw()
{
    m_window.display();
}

int SfmlAdapter::setDrawColour(uint8_t, // r
    uint8_t, // g
    uint8_t, // b
    uint8_t // a
)
{
    // TODO
    return 0;
}

void SfmlAdapter::drawLine(int xFrom,
    int yFrom,
    int xTo,
    int yTo,
    int /*width*/,
    int r,
    int g,
    int b)
{
    // Note, apparently SFML uses a rectangle shape to draw a line with
    // thickness, so this may need revisiting in future (TODO?)
    sf::Vertex line[]
        = { sf::Vertex(sf::Vector2f(xFrom, yFrom)), sf::Vertex(sf::Vector2f(xTo, yTo)) };
    line[0].color = sf::Color(r, g, b);
    line[1].color = sf::Color(r, g, b);
    m_window.draw(line, 2, sf::Lines);
}

void SfmlAdapter::fillRectangle(int, // x,
    int, // y,
    int, // w,
    int // h
)
{
    // TODO
}

int SfmlAdapter::getPhysicalScreenWidth()
{
    return sf::VideoMode::getDesktopMode().width;
}

int SfmlAdapter::getPhysicalScreenHeight()
{
    return sf::VideoMode::getDesktopMode().height;
}

void SfmlAdapter::loopDelay(uint32_t previousTicks, // milliseconds
    uint32_t totalLoopMilliseconds) const
{
    // TODO we could use the simpler SFML setFramerateLimit
    sf::Clock clock;
    while (clock.getElapsedTime().asMilliseconds()
        < static_cast<int>(previousTicks + totalLoopMilliseconds)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

size_t SfmlAdapter::imageLoad(const std::string& /* fileName */)
{
    return 0; // TODO DELETE?
}

void SfmlAdapter::imageDisplay(size_t, // id,
    int, // x,
    int // y
)
{
    // TODO - delete?
}

void SfmlAdapter::imageUnload(size_t /* id */)
{
    // TODO - delete?
}

void SfmlAdapter::imageDisplay(const std::string&, // fileName,
    int, // x,
    int // y
)
{
    // TODO - delete?
}

void SfmlAdapter::registerControlHandler(KeyControls key,
    std::function<void(const bool, const float val)> controlHandler)
{
    m_controlHandlers[key] = controlHandler;
}

void SfmlAdapter::processInput(bool paused)
{
    sf::Event event;

    while (m_window.pollEvent(event)) {
        if (sf::Joystick::isConnected(0)) {
            float x = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
            m_controlHandlers[KeyControls::LR_ANALOGUE](true, -x);
            // Right trigger
            float v = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::V);
            if (v > -90.f) {
                m_controlHandlers[KeyControls::ACCELERATE](true, (v + 100.f) / 2.f);
            } else {
                m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
            }
        }
        switch (event.type) {
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                    case sf::Keyboard::Escape:
                        m_controlHandlers[KeyControls::QUIT](true, 0.f);
                        break;
                    case sf::Keyboard::Up:
                    case sf::Keyboard::Space:
                        if (!paused) {
                            m_controlHandlers[KeyControls::ACCELERATE](true, 25.f);
                        }
                        break;
                    case sf::Keyboard::Left:
                    case sf::Keyboard::A:
                    case sf::Keyboard::Comma:
                        if (!paused) {
                            m_controlHandlers[KeyControls::LEFT](true, 0.f);
                        }
                        break;
                    case sf::Keyboard::Right:
                    case sf::Keyboard::D:
                    case sf::Keyboard::Period:
                        m_controlHandlers[KeyControls::RIGHT](true, 0.f);
                        if (!paused) {
                            break;
                        }
                    case sf::Keyboard::P:
                        m_controlHandlers[KeyControls::PAUSE](true, 0.f);
                        break;
                    default:
                        break;
                }
                break;
            case sf::Event::KeyReleased:
                switch (event.key.code) {
                    case sf::Keyboard::Up:
                    case sf::Keyboard::Space:
                        m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                        break;
                    case sf::Keyboard::Left:
                    case sf::Keyboard::A:
                    case sf::Keyboard::Comma:
                        m_controlHandlers[KeyControls::LEFT](false, 0.f);
                        break;
                    case sf::Keyboard::Right:
                    case sf::Keyboard::D:
                    case sf::Keyboard::Period:
                        m_controlHandlers[KeyControls::RIGHT](false, 0.f);
                        break;
                    default:
                        break;
                }
                break;

            // we don't process other types of events currently
            default:
                break;
        }
    }
    return;
}

void SfmlAdapter::soundLoad(const std::string& key, const std::string& filename)
{
    auto buffer = std::make_shared<sf::SoundBuffer>();
    if (!buffer->loadFromFile(filename)) {
        THROWUP(AmazeRuntimeException, "Sound file load error");
    }
    m_soundBuffers[key] = buffer;
    auto sound = std::make_shared<sf::Sound>();
    sound->setBuffer(*buffer);
    m_sounds[key] = sound;
}

void SfmlAdapter::soundPlay(const std::string& key)
{
    m_sounds[key]->setVolume(100.f);
    if (m_sounds[key]->getStatus() != sf::Sound::Playing) {
        m_sounds[key]->play();
    }
}

void SfmlAdapter::soundLoop(const std::string& key, float volume)
{
    m_sounds[key]->setVolume(volume);
    if (m_sounds[key]->getStatus() != sf::Sound::Playing) {
        m_sounds[key]->setLoop(true);
        m_sounds[key]->play();
    }
}

void SfmlAdapter::soundFade(const std::string& key, const int /* msecs */)
{
    // We don't have a fadeOut() type function in SFML so we reduce a bit on
    // each call
    float newVolume = m_sounds[key]->getVolume() - 3.f;
    if (newVolume < 0.f) {
        newVolume = 0.f;
    }
    m_sounds[key]->setVolume(newVolume);
}

bool SfmlAdapter::isFullScreenSafe()
{
    return !sf::VideoMode::getFullscreenModes().empty();
}

} // namespace amaze
} // namespace marengo
