#include "controller.h"
#include "audio.h"
#include "gamemodel.h"
#include "gameshape.h"
#include "levelprogress.h"
#include "log.h" // IWYU pragma: keep
#include "settings.h"
#include "utils.h"

#include <algorithm>
#include <filesystem>
#include <memory>

namespace marengo {
namespace amaze {

namespace {

// How levels are identified in the progress (see LevelProgress)
std::string levelName(const std::string& filename)
{
    return std::filesystem::path(filename).filename().string();
}

constexpr std::chrono::nanoseconds TICK_DURATION { 1'000'000'000 / TICKS_PER_SECOND };
// If a frame takes longer than this (e.g. window being set up, dragged, level
// loading), don't try to catch up the lost time; the game just pauses briefly.
// Catching up would make everything jump, and the player would have had no
// chance to react during the hiccup anyway. This still allows for displays
// down to 20Hz.
constexpr std::chrono::milliseconds MAX_FRAME_TIME { 50 };
// Log frames or input processing that take longer than this (diagnostic only)
constexpr std::chrono::milliseconds STALL_LOG_THRESHOLD { 50 };
// Gamepad rumble when the ship scrapes an obstacle and survives, from 0 to 1.
// Harder impacts rumble more, up to full strength for this speed (units per
// tick) or faster.
constexpr double SCRAPE_RUMBLE_INTENSITY = 0.5;
constexpr double SCRAPE_RUMBLE_FULL_SPEED = 3.0;
constexpr uint32_t SCRAPE_RUMBLE_MS = 200;
// Short rumble when the rumble intensity is changed in the menu, so the new
// strength can be felt
constexpr uint16_t RUMBLE_PREVIEW_INTENSITY = 0xC000;
constexpr uint32_t RUMBLE_PREVIEW_MS = 150;
// How much of the shield each fuel object restores
constexpr double FUEL_SHIELD_RECHARGE = 0.1;

} // namespace

Controller::Controller(
    GameModel& m,
    View& v,
    GameShell& shell,
    Audio& audio,
    Settings& settings,
    LevelProgress& progress)
    : m_gameModel(m)
    , m_view(v)
    , m_shell(shell)
    , m_audio(audio)
    , m_settings(settings)
    , m_progress(progress)
    , m_rumbleIntensityPercent(shell.menuResults().rumbleIntensityPercent)
{
    registerControlHandlers();

    std::filesystem::path dataPath(m_gameModel.getDataPath());

    // Load sounds...
    m_audio.soundLoad("rocket", (dataPath / "rocket.wav").string());
    m_audio.soundLoad("collision", (dataPath / "collision.wav").string());
    m_audio.soundLoad("collect", (dataPath / "collect.wav").string());
    m_audio.soundLoad("success", (dataPath / "success.wav").string());
    m_audio.soundLoad("breakable", (dataPath / "breakable.wav").string());
    m_audio.soundLoad("scrape", (dataPath / "scrape.wav").string());
    m_audio.musicLoad((dataPath / "music.ogg").string());
}

void Controller::registerControlHandlers()
{
    // We register callbacks with the game shell, which maps keys and gamepad
    // input to these controls, so key codes etc. don't appear outside it.

    // Rotate Left
    m_shell.registerControlHandler(KeyControls::LEFT, [&](const bool isKeyDown, const float value) {
        if (isKeyDown) {
            startPlayIfReady();
            m_gameModel.shipModel().setRotationDelta(value);
        } else {
            // if we are already rotating in this direction:
            if (utils::sgn(m_gameModel.shipModel().rotationDelta()) == 1) {
                m_gameModel.shipModel().setRotationDelta(0);
            }
        }
    });

    // Rotate with analogue stick
    m_shell.registerControlHandler(
        KeyControls::LR_ANALOGUE, [&](const bool /* isKeyDown */, const float value) {
            double rotationDelta = value;
            if (std::abs(rotationDelta) < 0.01) {
                rotationDelta = 0.0;
            } else {
                startPlayIfReady();
            }
            m_gameModel.shipModel().setRotationDelta(rotationDelta);
        });

    // Rotate Right
    m_shell.registerControlHandler(
        KeyControls::RIGHT, [&](const bool isKeyDown, const float value) {
            if (isKeyDown) {
                startPlayIfReady();
                m_gameModel.shipModel().setRotationDelta(value);
            } else {
                // if we are already rotating in this direction:
                if (utils::sgn(m_gameModel.shipModel().rotationDelta()) == -1) {
                    m_gameModel.shipModel().setRotationDelta(0);
                }
            }
        });

    // Accelerate
    m_shell.registerControlHandler(
        KeyControls::ACCELERATE, [&](const bool isKeyDown, const float acceleration) {
            if (isKeyDown) {
                startPlayIfReady();
                float accln = acceleration / 1000.f;
                m_gameModel.shipModel().setIsAccelerating(true, accln);
            } else {
                m_gameModel.shipModel().setIsAccelerating(false, 0.f);
            }
        });

    // Pause
    m_shell.registerControlHandler(KeyControls::PAUSE, [&](const bool isKeyDown, const float) {
        int now = m_shell.ticks();
        if (isKeyDown
            && (m_gameModel.getGameState() == GameState::Running
                || m_gameModel.getGameState() == GameState::Paused)
            && now > m_lastPause + 500) {
            m_gameModel.togglePause();
            m_lastPause = m_shell.ticks();
        }
    });

    // Any other key or button which isn't a control in its own right
    m_shell.registerControlHandler(KeyControls::START, [&](const bool isKeyDown, const float) {
        if (isKeyDown) {
            startPlayIfReady();
        }
    });

    // Menu
    m_shell.registerControlHandler(KeyControls::MENU, [&](const bool, const float) {
        m_gameModel.setGameState(GameState::Menu);
    });

    // Quit
    m_shell.registerControlHandler(KeyControls::QUIT, [&](const bool isKeyDown, const float) {
        if (isKeyDown) {
            m_gameModel.setGameState(GameState::Quit);
        }
    });
}

void Controller::start(const std::string& levelFile)
{
    // TODO splash screen?
    m_audio.musicPlayLoop();
    m_audio.setMusicVolumePercent(m_shell.menuResults().musicVolumePercent);

    loadLevel(levelFile);
    m_levelStartPending = true;
    beginLevel();

    m_lastFrameTime = std::chrono::steady_clock::now();
    m_accumulator = {};
}

bool Controller::frame(QPainter& painter, const QSizeF& canvasSize)
{
    using namespace std::chrono;

    auto beforeInput = steady_clock::now();
    auto elapsed = beforeInput - m_lastFrameTime;
    if (elapsed > STALL_LOG_THRESHOLD) {
        mgo::Log::debug(
            std::format("Frame interval of {}ms", duration_cast<milliseconds>(elapsed).count()));
    }
    m_accumulator += std::min<steady_clock::duration>(elapsed, MAX_FRAME_TIME);

    processInput();

    // Time spent processing input is excluded from simulation time. Event polling
    // can block (e.g. on macOS the first key press can take a few hundred ms while
    // text input services initialise); counting that time would make the
    // simulation catch up in one burst with the new input already applied,
    // causing e.g. a sudden jump in rotation.
    auto afterInput = steady_clock::now();
    if (afterInput - beforeInput > STALL_LOG_THRESHOLD) {
        mgo::Log::debug(
            std::format(
                "Input processing took {}ms",
                duration_cast<milliseconds>(afterInput - beforeInput).count()));
    }
    m_lastFrameTime = afterInput;

    while (m_accumulator >= TICK_DURATION) {
        if (m_gameModel.getGameState() == GameState::Quit) {
            return false;
        }
        tick();
        m_accumulator -= TICK_DURATION;
    }
    if (m_gameModel.getGameState() == GameState::Quit) {
        return false;
    }

    render(painter, canvasSize);
    return true;
}

void Controller::processInput()
{
    switch (m_gameModel.getGameState()) {
        case GameState::Menu:
            processMenu();
            break;
        case GameState::Paused:
            m_shell.processInput(true);
            break;
        case GameState::GetReady:
        case GameState::Running:
            m_shell.setMouseCursorVisible(false);
            m_shell.processInput(false);
            break;
        default:
            break;
    }
}

void Controller::beginLevel()
{
    // Wait for the player to press something before play starts. Besides
    // giving them time to look at the level, any one-off delays (window being
    // set up at startup; on macOS the first key press can stall for 100ms+)
    // then happen while nothing is moving.
    m_gameModel.setGameState(GameState::GetReady);
}

void Controller::loadLevel(const std::string& filename)
{
    m_gameModel.levelLoad(filename);
    m_gameModel.setShield(m_progress.startingShield(levelName(filename)));
    m_settings.setLastLevelFile(m_gameModel.levelFileName());
}

void Controller::levelCompleted()
{
    if (m_progress.complete(levelName(m_gameModel.levelFileName()), m_gameModel.shield())) {
        m_settings.setCompletedLevels(m_progress.completedLevels());
        m_settings.setCarriedShield(m_progress.carriedShield());
    }
}

void Controller::loadLevelAfterSuccess()
{
    // The next level in order, or this one again if it's the last (or isn't
    // one of the game's levels)
    const std::string current = m_gameModel.levelFileName();
    if (auto next = m_progress.levelAfter(levelName(current))) {
        loadLevel((std::filesystem::path(current).parent_path() / *next).string());
    } else {
        loadLevel(current);
    }
}

void Controller::saveMenuOptions()
{
    MenuResults results = m_shell.menuResults();
    m_settings.setMusicVolumePercent(results.musicVolumePercent);
    m_settings.setStickDeadZonePercent(results.stickDeadZonePercent);
    m_settings.setRumbleIntensityPercent(results.rumbleIntensityPercent);
}

void Controller::startPlayIfReady()
{
    // Called from control handlers; the control which starts play also takes
    // effect, e.g. pressing rotate starts the level already rotating
    if (m_gameModel.getGameState() == GameState::GetReady) {
        m_levelStartPending = false;
        m_gameModel.setGameState(GameState::Running);
    }
}

void Controller::processMenu()
{
    m_shell.setMouseCursorVisible(true);
    if (m_currentMenu == MenuType::None) {
        m_currentMenu = MenuType::Main;
        m_shell.rumble(0, 0, 0); // turn off any rumbles
        m_gameModel.shipModel().setIsAccelerating(false, 0.f);
        m_view.stopSounds();
        // Which levels can be chosen, with the current one selected
        m_shell.updateMenuLevels(m_progress, m_gameModel.levelFileName());
    }
    m_shell.processMenuInput();
    // Adjust background music and rumble intensity immediately
    m_audio.setMusicVolumePercent(m_shell.menuResults().musicVolumePercent);
    int rumblePercent = m_shell.menuResults().rumbleIntensityPercent;
    if (rumblePercent != m_rumbleIntensityPercent) {
        m_rumbleIntensityPercent = rumblePercent;
        m_shell.setRumbleIntensityPercent(rumblePercent);
        m_shell.rumble(RUMBLE_PREVIEW_INTENSITY, RUMBLE_PREVIEW_INTENSITY, RUMBLE_PREVIEW_MS);
    }
    if (m_currentMenu == MenuType::OK || m_currentMenu == MenuType::Cancel) {
        // User closed dialog with OK or Cancel. (Like the music volume, the
        // dead zone applies even on Cancel: these options take effect as
        // they're changed rather than on OK.)
        MenuResults results = m_shell.menuResults();
        m_shell.setStickDeadZonePercent(results.stickDeadZonePercent);
        saveMenuOptions();
        if (m_currentMenu == MenuType::OK) {
            // Don't restart the current level, e.g. when the menu was only
            // opened to change the music volume
            std::error_code ec;
            bool isCurrentLevel = std::filesystem::equivalent(
                results.levelFileToLoad, m_gameModel.levelFileName(), ec);
            if (!results.levelFileToLoad.empty() && !isCurrentLevel) {
                loadLevel(results.levelFileToLoad);
                m_levelStartPending = true;
            }
        }
        m_currentMenu = MenuType::None;
        m_gameModel.shipModel().setVisible(true);
        if (m_levelStartPending) {
            beginLevel();
        } else {
            m_gameModel.setGameState(GameState::Running);
        }
    }
    if (m_currentMenu == MenuType::Exit) {
        saveMenuOptions();
        m_gameModel.setGameState(GameState::Quit);
    }
}

void Controller::tick()
{
    m_scheduler.processSchedule();
    switch (m_gameModel.getGameState()) {
        case GameState::Paused:
            m_shell.rumble(0, 0, 0); // turn off any rumbles
            m_view.stopSounds();
            break;
        case GameState::Dead:
            // We get here when the ship has finished exploding
            m_shell.rumble(0, 0, 0);
            m_scheduler.doAfter(ScheduleEventName::LevelEnded, 300, [&]() {
                // We just reload the level, but display the menu
                // in case the user wants a different level
                loadLevel(m_gameModel.levelFileName());
                m_levelStartPending = true;
                m_gameModel.shipModel().setVisible(false);
                m_gameModel.setGameState(GameState::Menu);
            });
            break;
        case GameState::Succeeded:
            m_view.stopSounds();
            m_shell.rumble(0, 0, 0); // turn off any rumbles
            m_gameModel.shipModel().shipGameShape()->resize(1.2);
            m_gameModel.shipModel().setIsAccelerating(false);
            m_gameModel.shipModel().flamesGameShape()->setVisible(false);
            m_gameModel.setBreakableExploding(false);
            m_scheduler.doAfter(ScheduleEventName::LevelEnded, 300, [&]() {
                // On to the next level, but display the menu in case the
                // user wants a different one
                loadLevelAfterSuccess();
                m_levelStartPending = true;
                m_gameModel.shipModel().setVisible(false);
                m_gameModel.setGameState(GameState::Menu);
            });
            break;
        case GameState::Exploding:
            m_shell.rumble(0xFFFF, 0xFFFF, 1000);
            m_gameModel.tick();
            break;
        case GameState::Running:
            m_gameModel.tick();
            collisionChecks();
            break;
        default:
            break;
    }
    m_view.playSounds();
}

void Controller::render(QPainter& painter, const QSizeF& canvasSize)
{
    m_view.draw(painter, canvasSize);
    if (m_gameModel.getGameState() == GameState::Menu) {
        m_currentMenu = m_shell.updateMenu(m_currentMenu);
    }
}

void Controller::collisionChecks()
{
    // Collision detection
    auto colliderTuple = m_gameModel.collisionDetect();
    // collider is a tuple of GameShapeType and shared_ptr<GameShape>
    std::shared_ptr<GameShape> collider = std::get<std::shared_ptr<GameShape>>(colliderTuple);
    GameShapeType collideeType = std::get<GameShapeType>(colliderTuple);
    if (collider) {
        switch (collider->getGameShapeType()) {
            case GameShapeType::EXIT:
                if (collideeType != GameShapeType::FLAMES) {
                    m_audio.soundPlay("success");
                    m_gameModel.setGameState(GameState::Succeeded);
                    levelCompleted();
                }
                break;
            case GameShapeType::FUEL:
                if (collideeType != GameShapeType::FLAMES) {
                    m_audio.soundPlay("collect");
                    collider->setIsActive(false);
                    m_gameModel.rechargeShield(FUEL_SHIELD_RECHARGE);
                }
                break;
            case GameShapeType::KEY:
                // currently an idea but not used
                break;
            case GameShapeType::NEUTRAL:
                break;
            case GameShapeType::OBSTRUCTION:
            case GameShapeType::SPECIAL:
                if (collideeType != GameShapeType::FLAMES) {
                    bounceShip();
                }
                break;
            case GameShapeType::BREAKABLE:
                if (collideeType != GameShapeType::FLAMES) {
                    // Bounce before it's destroyed, as only active objects
                    // are bounced off
                    bounceShip();
                }
                m_audio.soundPlay("breakable");
                m_gameModel.setBreakableExploding();
                collider->setIsActive(false); // Breakable objects can be destroyed
                break;
            case GameShapeType::PRISONER:
                // currently an idea but not used
                break;
            case GameShapeType::UNINITIALISED:
                break;
            default:
                break;
        }
    }
}

void Controller::bounceShip()
{
    Impact impact = m_gameModel.bounceShip();
    m_gameModel.shieldImpact(impact);
    if (m_gameModel.shield() <= 0.0) {
        m_gameModel.shipModel().setIsExploding(true);
        m_gameModel.setGameState(GameState::Exploding);
        return;
    }
    // The ship survived the contact. (soundPlay() doesn't restart the sound
    // if it's still playing, so sustained contact doesn't stutter. Each rumble
    // replaces the last, so the rumble lasts as long as the contact, plus
    // SCRAPE_RUMBLE_MS.)
    m_audio.soundPlay("scrape");
    double strength = std::clamp(
        impact.speed / SCRAPE_RUMBLE_FULL_SPEED, SCRAPE_RUMBLE_INTENSITY, 1.0);
    auto intensity = static_cast<uint16_t>(strength * 0xFFFF);
    m_shell.rumble(intensity, intensity, SCRAPE_RUMBLE_MS);
}

} // namespace amaze
} // namespace marengo
