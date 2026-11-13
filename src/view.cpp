#include "view.h"
#include "shape.h"
#include "utils.h"

#include <algorithm>
#include <cmath>

namespace marengo {
namespace amaze {

View::View(GameModel& model, IGraphicsAdapter& gm)
    : m_model(model)
    , m_graphicsAdapter(gm)
{
}

void View::playSounds()
{
    // (No rocket noise while waiting to start, even if thrust is held)
    if (m_model.getShipModel()->isAccelerating() && m_model.getGameState() != GameState::GetReady) {
        float vol = m_model.getShipModel()->accelerationAmount() * 2000.f;
        vol = std::clamp(vol, 2.f, 100.f);

        m_graphicsAdapter.soundLoop("rocket", vol);
    } else {
        m_graphicsAdapter.soundFade("rocket", 1000);
    }

    if (m_model.getGameState() == GameState::Exploding) {
        m_graphicsAdapter.soundPlay("collision");
    }
}

void View::stopSounds()
{
    m_graphicsAdapter.soundFade("rocket", 400);
}

void View::draw()
{
    // TODO: this should take a model as its parameter; we shouldn't be bound
    // to one model but should be able to accept any model implementing a
    // specific interface. This would enable us to use models for splash
    // screens / high score screens etc.

    // This is where all state is "realised" as images drawn. (Sounds are handled
    // separately in playSounds(), which is called at the fixed simulation rate.)

    // Dynamic shapes (i.e. shapes which rotate around the ship)
    for (const auto& shape : m_model.getAllDynamicObjects()) {
        if (shape->isVisible() && shape->IsActive()) {
            rotateAndDrawShape(*shape);
        }
    }

    // Draw black rectangle at top of screen for status bar (and hiding the "notch" on
    // macs in full screen)
    m_graphicsAdapter.drawStatusBar();

    // Lives remaining
    Text t;
    t.g = 128;
    t.characterSize = 40;
    t.positionX = 30;
    t.positionY = 20;
    t.text = "Ships remaining: " + std::to_string(m_model.getLivesRemaining());
    m_graphicsAdapter.drawText(t);

    if (m_model.getGameState() == GameState::GetReady) {
        Text gr;
        gr.r = 255;
        gr.g = 255;
        gr.characterSize = 160;
        gr.text = "Get Ready";
        gr.positionY = m_graphicsAdapter.getWindowHeight() * 0.3;
        m_graphicsAdapter.drawText(gr);
        if (!m_model.levelDescription().empty()) {
            Text desc;
            desc.g = 192;
            desc.characterSize = 60;
            desc.text = m_model.levelDescription();
            desc.positionY = m_graphicsAdapter.getWindowHeight() * 0.3 + 200;
            m_graphicsAdapter.drawText(desc);
        }
        Text prompt;
        prompt.r = 160;
        prompt.g = 160;
        prompt.b = 160;
        prompt.characterSize = 40;
        prompt.text = "Press any key to start";
        prompt.positionY = m_graphicsAdapter.getWindowHeight() * 0.3 + 300;
        m_graphicsAdapter.drawText(prompt);
    }

    if (m_model.gameIsPaused()) {
        Text pm;
        pm.r = 255;
        pm.g = 255;
        pm.characterSize = 160;
        pm.text = "Paused";
        pm.positionY = m_graphicsAdapter.getWindowHeight() * 0.3;
        m_graphicsAdapter.drawText(pm);
        // TODO do we need pause if opening the menu does the same thing?
    }

    if (m_model.getGameState() == GameState::Succeeded) {
        Text pm;
        pm.r = 255;
        pm.g = 255;
        pm.characterSize = 160;
        pm.text = "Success!";
        pm.positionY = m_graphicsAdapter.getWindowHeight() * 0.3;
        m_graphicsAdapter.drawText(pm);
    }

    if (m_model.getGameState() == GameState::Dead && m_model.getLivesRemaining() == 0) {
        Text pm;
        pm.r = 255;
        pm.characterSize = 160;
        pm.text = "Game Over";
        pm.positionY = m_graphicsAdapter.getWindowHeight() * 0.3;
        m_graphicsAdapter.drawText(pm);
    }
}

void View::rotateAndDrawShape(const GameShape& shape) const
{
    // The viewport always shows the same area of the arena (480x300 units
    // worth), regardless of its physical dimensions, so a differently-shaped
    // window doesn't let the player see more of the maze:
    double width = m_graphicsAdapter.getWindowWidth();
    double height = m_graphicsAdapter.getWindowHeight();
    double scale = std::sqrt((width * height) / (480.0 * 300.0));
    double xOffset = m_graphicsAdapter.getWindowWidth() / 2.0;
    double yOffset = m_graphicsAdapter.getWindowHeight() / 2.0;

    for (const auto& sl : shape.getVec()) {
        double x0 = sl.x0 + shape.getPosX() - m_model.getShipModel()->x();
        double y0 = sl.y0 + shape.getPosY() - m_model.getShipModel()->y();
        double x1 = sl.x1 + shape.getPosX() - m_model.getShipModel()->x();
        double y1 = sl.y1 + shape.getPosY() - m_model.getShipModel()->y();
        // OK, when we get here, we have a line expressed
        // relative to the origin OF THE SHIP.
        double dCos = utils::cosine(m_model.getShipModel()->rotation());
        double dSin = utils::sine(m_model.getShipModel()->rotation());
        double x0r, y0r, x1r, y1r;
        x0r = x0 * dCos - y0 * dSin;
        y0r = x0 * dSin + y0 * dCos;
        x1r = x1 * dCos - y1 * dSin;
        y1r = x1 * dSin + y1 * dCos;
        // now draw adjusted for physical screen coords
        uint8_t r = sl.r;
        uint8_t g = sl.g;
        uint8_t b = sl.b;
        // Special case for objects with gravity, we add random colours
        if (shape.getGravity() != 0.f) {
            r = utils::rnd(193);
            g = utils::rnd(193);
            b = utils::rnd(193);
        }
        if (m_model.getGameState() == GameState::Menu
            || m_model.getGameState() == GameState::Paused) {
            // Dim everything
            r *= 0.4;
            g *= 0.4;
            b *= 0.4;
        }
        m_graphicsAdapter.drawLine(
            x0r * scale + xOffset,
            y0r * scale + yOffset,
            x1r * scale + xOffset,
            y1r * scale + yOffset,
            sl.lineThickness * m_graphicsAdapter.getScalingFactor(),
            r,
            g,
            b);
    }
}

} // namespace amaze
} // namespace marengo
