#pragma once

// Settings remembered between runs, stored via QSettings in the platform's
// usual place (e.g. ~/Library/Preferences on macOS). Values which have never
// been saved are returned as empty optionals, so callers can fall back to
// defaults (e.g. from amaze.cfg). The application's name must be set (via
// QCoreApplication) before this is constructed.

#include <QRect>
#include <QSettings>

#include <optional>
#include <string>
#include <vector>

namespace marengo {
namespace amaze {

class Settings {
public:
    std::optional<int> musicVolumePercent() const;
    void setMusicVolumePercent(int value);

    std::optional<int> stickDeadZonePercent() const;
    void setStickDeadZonePercent(int value);

    std::optional<int> rumbleIntensityPercent() const;
    void setRumbleIntensityPercent(int value);

    // File names (without directories) of the levels completed, and the
    // shield left after the last new level was completed: see LevelProgress
    std::vector<std::string> completedLevels() const;
    void setCompletedLevels(const std::vector<std::string>& value);
    std::optional<double> carriedShield() const;
    void setCarriedShield(std::optional<double> value);

    // Full path of the level file last played
    std::optional<std::string> lastLevelFile() const;
    void setLastLevelFile(const std::string& value);

    // The window's normal (i.e. not full screen) geometry
    std::optional<QRect> windowGeometry() const;
    void setWindowGeometry(const QRect& value);

    std::optional<bool> fullScreen() const;
    void setFullScreen(bool value);

private:
    template <typename T> std::optional<T> get(const char* key) const;

    QSettings m_settings;
};

} // namespace amaze
} // namespace marengo
