#include "settings.h"

namespace marengo {
namespace amaze {

namespace {

constexpr const char* MUSIC_VOLUME = "audio/musicVolumePercent";
constexpr const char* STICK_DEAD_ZONE = "gamepad/stickDeadZonePercent";
constexpr const char* RUMBLE_INTENSITY = "gamepad/rumbleIntensityPercent";
constexpr const char* LAST_LEVEL_FILE = "game/lastLevelFile";
constexpr const char* WINDOW_GEOMETRY = "window/geometry";
constexpr const char* FULL_SCREEN = "window/fullScreen";

} // namespace

template <typename T> std::optional<T> Settings::get(const char* key) const
{
    QVariant value = m_settings.value(key);
    if (!value.isValid() || !value.canConvert<T>()) {
        return std::nullopt;
    }
    return value.value<T>();
}

std::optional<int> Settings::musicVolumePercent() const
{
    return get<int>(MUSIC_VOLUME);
}

void Settings::setMusicVolumePercent(int value)
{
    m_settings.setValue(MUSIC_VOLUME, value);
}

std::optional<int> Settings::stickDeadZonePercent() const
{
    return get<int>(STICK_DEAD_ZONE);
}

void Settings::setStickDeadZonePercent(int value)
{
    m_settings.setValue(STICK_DEAD_ZONE, value);
}

std::optional<int> Settings::rumbleIntensityPercent() const
{
    return get<int>(RUMBLE_INTENSITY);
}

void Settings::setRumbleIntensityPercent(int value)
{
    m_settings.setValue(RUMBLE_INTENSITY, value);
}

std::optional<std::string> Settings::lastLevelFile() const
{
    auto value = get<QString>(LAST_LEVEL_FILE);
    if (!value.has_value() || value->isEmpty()) {
        return std::nullopt;
    }
    return value->toStdString();
}

void Settings::setLastLevelFile(const std::string& value)
{
    m_settings.setValue(LAST_LEVEL_FILE, QString::fromStdString(value));
}

std::optional<QRect> Settings::windowGeometry() const
{
    auto value = get<QRect>(WINDOW_GEOMETRY);
    if (!value.has_value() || !value->isValid()) {
        return std::nullopt;
    }
    return value;
}

void Settings::setWindowGeometry(const QRect& value)
{
    m_settings.setValue(WINDOW_GEOMETRY, value);
}

std::optional<bool> Settings::fullScreen() const
{
    return get<bool>(FULL_SCREEN);
}

void Settings::setFullScreen(bool value)
{
    m_settings.setValue(FULL_SCREEN, value);
}

} // namespace amaze
} // namespace marengo
