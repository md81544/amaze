#pragma once

// Sound effects and background music. A QApplication must exist before this
// is constructed.

#include <QAudioOutput>
#include <QMediaPlayer>

#include <memory>
#include <string>
#include <unordered_map>

class QSoundEffect;

namespace marengo {
namespace amaze {

class Audio {
public:
    Audio();
    ~Audio();

    // Load a sound & supply a key to reference it
    void soundLoad(const std::string& key, const std::string& filename);
    // Play once at full volume, unless already playing
    void soundPlay(const std::string& key);
    // Loop at the given volume (percent), starting it if not already playing
    void soundLoop(const std::string& key, float volume);
    // Reduce the volume a little; call repeatedly to fade out
    void soundFade(const std::string& key);

    void musicLoad(const std::string& filename);
    void musicPlayLoop();
    void setMusicVolumePercent(float volume);

private:
    QSoundEffect& sound(const std::string& key);

    std::unordered_map<std::string, std::unique_ptr<QSoundEffect>> m_sounds;
    QAudioOutput m_musicOutput;
    QMediaPlayer m_music;
};

} // namespace amaze
} // namespace marengo
