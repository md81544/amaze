#include "audio.h"
#include "exceptions.h"
#include "log.h"

#include <QFileInfo>
#include <QSoundEffect>

#include <algorithm>
#include <format>

namespace marengo {
namespace amaze {

namespace {

void checkFileExists(const std::string& filename)
{
    if (!QFileInfo::exists(QString::fromStdString(filename))) {
        THROWUP(AmazeRuntimeException, std::format("File not found ({})", filename));
    }
}

} // namespace

Audio::Audio()
{
    m_music.setAudioOutput(&m_musicOutput);
    QObject::connect(
        &m_music,
        &QMediaPlayer::errorOccurred,
        &m_music,
        [](QMediaPlayer::Error, const QString& message) {
            mgo::Log::error(std::format("Music playback error: {}", message.toStdString()));
        });
}

Audio::~Audio() = default;

QSoundEffect& Audio::sound(const std::string& key)
{
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) {
        THROWUP(AmazeRuntimeException, std::format("Sound not loaded ({})", key));
    }
    return *it->second;
}

void Audio::soundLoad(const std::string& key, const std::string& filename)
{
    checkFileExists(filename);
    auto effect = std::make_unique<QSoundEffect>();
    effect->setSource(QUrl::fromLocalFile(QString::fromStdString(filename)));
    m_sounds[key] = std::move(effect);
}

void Audio::soundPlay(const std::string& key)
{
    auto& s = sound(key);
    s.setVolume(1.f);
    if (!s.isPlaying()) {
        s.setLoopCount(1);
        s.play();
    }
}

void Audio::soundLoop(const std::string& key, float volume)
{
    auto& s = sound(key);
    s.setVolume(std::clamp(volume, 0.f, 100.f) / 100.f);
    if (!s.isPlaying()) {
        s.setLoopCount(QSoundEffect::Infinite);
        s.play();
    }
}

void Audio::soundFade(const std::string& key)
{
    auto& s = sound(key);
    s.setVolume(std::max(s.volume() - 0.03f, 0.f));
}

void Audio::musicLoad(const std::string& filename)
{
    checkFileExists(filename);
    m_music.setSource(QUrl::fromLocalFile(QString::fromStdString(filename)));
}

void Audio::musicPlayLoop()
{
    m_music.setLoops(QMediaPlayer::Infinite);
    m_musicOutput.setVolume(1.f);
    m_music.play();
}

void Audio::setMusicVolumePercent(float volume)
{
    m_musicOutput.setVolume(std::clamp(volume, 0.f, 100.f) / 100.f);
}

} // namespace amaze
} // namespace marengo
