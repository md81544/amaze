#include "sfmladapter.h"
#include "exceptions.h"
#include "helperfunctions.h"
#include "log.h"

#include <chrono>
#include <thread>

namespace marengo {
namespace amaze {

namespace {

KeyControls joystickRateLimiter(KeyControls key)
{
    static bool firstCall = true;
    static auto lastCall = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    KeyControls rc = KeyControls::NONE;
    if (firstCall || now - lastCall > std::chrono::milliseconds(100)) {
        rc = key;
    }
    firstCall = false;
    lastCall = now;
    return rc;
}

}

SfmlAdapter::SfmlAdapter(
    int screenWidth,
    int screenHeight,
    bool useFullScreen,
    const std::string& dataDir)
    : m_window(
          useFullScreen ? sf::VideoMode::getDesktopMode()
                        : sf::VideoMode(screenWidth, screenHeight),
          "Amaze",
          useFullScreen ? sf::Style::Fullscreen : sf::Style::Default)
    , m_screenHeight(screenHeight)
    , m_screenWidth(screenWidth)
{
    m_window.setMouseCursorVisible(false);
    if (useFullScreen) {
        auto dm = sf::VideoMode::getDesktopMode();
        m_screenHeight = dm.height;
        m_screenWidth = dm.width;
    }
    m_font.loadFromFile(dataDir + "/Oxanium-SemiBold.ttf");
}

SfmlAdapter::~SfmlAdapter() { }

void SfmlAdapter::setFrameRate(unsigned int fr)
{
    m_window.setFramerateLimit(fr);
}

int SfmlAdapter::getWindowHeight() const
{
    return m_screenHeight;
}

int SfmlAdapter::getTicks() const
{
    return m_clock.getElapsedTime().asMilliseconds();
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

int SfmlAdapter::setDrawColour(
    uint8_t, // r
    uint8_t, // g
    uint8_t, // b
    uint8_t // a
)
{
    // TODO
    return 0;
}

void SfmlAdapter::drawLine(int xFrom, int yFrom, int xTo, int yTo, int width, int r, int g, int b)
{
    sf::Vector2f point1 { static_cast<float>(xFrom), static_cast<float>(yFrom) };
    sf::Vector2f point2 { static_cast<float>(xTo), static_cast<float>(yTo) };
    sf::Vertex vertices[4];

    sf::Vector2f direction = point2 - point1;
    sf::Vector2f unitDirection
        = direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);
    sf::Vector2f unitPerpendicular(-unitDirection.y, unitDirection.x);

    sf::Vector2f offset = (width / 2.f) * unitPerpendicular;

    vertices[0].position = point1 + offset;
    vertices[1].position = point2 + offset;
    vertices[2].position = point2 - offset;
    vertices[3].position = point1 - offset;

    for (int i = 0; i < 4; ++i) {
        vertices[i].color = sf::Color(r, g, b);
    }
    m_window.draw(vertices, 4, sf::Quads);
}

void SfmlAdapter::fillRectangle(
    int, // x,
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

size_t SfmlAdapter::imageLoad(const std::string& /* fileName */)
{
    return 0; // TODO DELETE?
}

void SfmlAdapter::imageDisplay(
    size_t, // id,
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

void SfmlAdapter::drawStatusBar()
{
    sf::RectangleShape sb(sf::Vector2f(m_screenWidth, 75));
    sb.setFillColor(sf::Color::Black);
    m_window.draw(sb);
}

void SfmlAdapter::drawText(const Text& text)
{
    sf::Text t;
    t.setFont(m_font);
    t.setCharacterSize(text.characterSize);
    t.setString(text.text);
    t.setFillColor(sf::Color({ text.r, text.g, text.b }));
    // x and y position are optional, with no value signifying centered in that dimension
    sf::FloatRect textRect = t.getLocalBounds();
    if (!text.positionX.has_value() && !text.positionY.has_value()) {
        t.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        t.setPosition(m_window.getView().getCenter());
    } else if (!text.positionX.has_value()) {
        // Centre horizontally
        t.setOrigin(textRect.left + textRect.width / 2.0f, 0.f);
        t.setPosition(m_window.getView().getCenter().x, text.positionY.value());
    } else if (!text.positionY.has_value()) {
        // Centre vertically
        t.setOrigin(0.f, textRect.top + textRect.height / 2.0f);
        t.setPosition(text.positionX.value(), m_window.getView().getCenter().y);
    } else {
        // If we get here we must have values for both x and y
        t.setPosition({ text.positionX.value(), text.positionY.value() });
    }
    m_window.draw(t);
}

void SfmlAdapter::drawMenu(std::vector<MenuItem> menuItems, int currentlyHighlightedItem)
{
    if (menuItems.empty()) {
        return;
    }
    Text t;
    t.g = 255;
    t.positionY = 300;
    t.characterSize = 60;
    t.text = menuItems[0].menuName;
    drawText(t);
    t.positionY.value() += 40;
    int itemCount = 0;
    for (const auto& menuItem : menuItems) {
        if (itemCount == currentlyHighlightedItem) {
            t.r = 255;
            t.g = 255;
            t.b = 255;
        } else {
            t.r = 0;
            t.g = 150;
            t.b = 0;
        }
        t.characterSize = 50;
        t.text = menuItem.text;
        t.positionY.value() += 60;
        drawText(t);
        ++itemCount;
    }
}

void SfmlAdapter::imageDisplay(
    const std::string&, // fileName,
    int, // x,
    int // y
)
{
    // TODO - delete?
}

void SfmlAdapter::registerControlHandler(
    KeyControls key,
    std::function<void(const bool, const float val)> controlHandler)
{
    m_controlHandlers[key] = controlHandler;
}

void SfmlAdapter::processInput(bool paused)
{
    sf::Event event;

    while (m_window.pollEvent(event)) {
        if (sf::Joystick::isConnected(0)) {
            if (!paused) {
                float x = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
                m_controlHandlers[KeyControls::LR_ANALOGUE](true, -x);
                // Right trigger
                float v = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::V);
                if (v > -90.f) {
                    m_controlHandlers[KeyControls::ACCELERATE](true, (v + 100.f) / 10.f);
                } else {
                    m_controlHandlers[KeyControls::ACCELERATE](false, 0.f);
                }
            }
            if (sf::Joystick::isButtonPressed(0, 9)) {
                m_controlHandlers[KeyControls::MENU](true, 0.f);
            }
        }
        switch (event.type) {
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                    case sf::Keyboard::Escape:
                        if (paused) {
                            // Escape also unpauses
                            m_controlHandlers[KeyControls::PAUSE](true, 0.f);
                        } else {
                            m_controlHandlers[KeyControls::MENU](true, 0.f);
                        }
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
                        if (!paused) {
                            m_controlHandlers[KeyControls::RIGHT](true, 0.f);
                        }
                        break;
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

KeyControls SfmlAdapter::processMenuInput()
{
    // If this function is called, the menu is displayed, so we react differently
    // to various keys. Note only one event is returned per call
    sf::Event event;

    while (m_window.pollEvent(event)) {
        if (sf::Joystick::isConnected(0)) {
            float y = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::R);
            if (y > 30.f) {
                return joystickRateLimiter(KeyControls::DOWN);
            } else if (y < -30.f) {
                return joystickRateLimiter(KeyControls::UP);
            }
            if (sf::Joystick::isButtonPressed(0, 1)) {
                return joystickRateLimiter(KeyControls::ENTER);
            }
            if (sf::Joystick::isButtonPressed(0, 2)) {
                return joystickRateLimiter(KeyControls::EXIT);
            }
            // TODO - menu button to return KeyControls::EXIT
        }
        switch (event.type) {
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                    case sf::Keyboard::Escape:
                        return KeyControls::EXIT;
                    case sf::Keyboard::Enter:
                        return KeyControls::ENTER;
                    case sf::Keyboard::Down:
                        return KeyControls::DOWN;
                    case sf::Keyboard::Up:
                        return KeyControls::UP;
                    default:
                        break;
                }
            // we don't process other types of events currently
            default:
                break;
        }
    }
    return KeyControls::NONE;
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

} // namespace amaze
} // namespace marengo
