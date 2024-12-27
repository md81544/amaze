#include "view.h"
#include "helperfunctions.h"
#include "log.h"
#include "shape.h"

#include <vector>

namespace marengo {
namespace amaze {

View::View(GameModel& model, IGraphicsAdapter& gm)
    : m_model(model)
    , m_graphicsAdapter(gm)
{
}

void View::playSounds()
{
    if (m_model.getShipModel()->isAccelerating()) {
        float vol = m_model.getShipModel()->accelerationAmount() * 1000.f;
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
    // This is currently just called when we go into paused state
    m_graphicsAdapter.soundFade("rocket", 400);
}

void View::update()
{
    // TODO: this should take a model as its parameter; we shouldn't be bound
    // to one model but should be able to accept any model implementing a
    // specific interface. This would enable us to use models for splash
    // screens / high score screens etc.

    // This is where all state is "realised" i.e. images drawn, sounds played
    // etc.
    playSounds();

    // Dynamic shapes (i.e. shapes which rotate around the ship)
    m_model.processDynamicObjects([&](GameShape& shape) {
        if (shape.isVisible() && shape.IsActive()) {
            if (shape.getGameShapeType() == GameShapeType::MOVING) {
                shape.move();
            }
            rotateAndDrawShape(shape);
        }
    });

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

    if (m_model.getGameState() == GameState::Menu) {
        auto vec = m_model.getCurrentMenu();
        m_graphicsAdapter.drawMenu(vec, m_model.getCurrentMenuItem());
    }
}

void View::rotateAndDrawShape(const GameShape& shape) const
{
    // We treat the viewport as representing 480 coordinate units wide,
    // regardless of its physical dimensions:
    double scale = m_graphicsAdapter.getWindoWidth() / 480.0;
    double xOffset = m_graphicsAdapter.getWindoWidth() / 2;
    double yOffset = m_graphicsAdapter.getWindowHeight() / 2;

    for (const auto& sl : shape.getVec()) {
        // ShapeLine sl = shape.getVec()[ line ];
        double x0 = sl.x0 + shape.getPosX() - m_model.getShipModel()->x();
        double y0 = sl.y0 + shape.getPosY() - m_model.getShipModel()->y();
        double x1 = sl.x1 + shape.getPosX() - m_model.getShipModel()->x();
        double y1 = sl.y1 + shape.getPosY() - m_model.getShipModel()->y();
        // OK, when we get here, we have a line expressed
        // relative to the origin of the ship.  We can apply the
        // rotate now (unless the shape is marked "don't rotate")...
        double dCos = helperfunctions::Cosine(m_model.getShipModel()->rotation());
        double dSin = helperfunctions::Sine(m_model.getShipModel()->rotation());
        double x0r, y0r, x1r, y1r;
        x0r = x0 * dCos - y0 * dSin;
        y0r = x0 * dSin + y0 * dCos;
        x1r = x1 * dCos - y1 * dSin;
        y1r = x1 * dSin + y1 * dCos;
        // now draw adjusted for physical screen coords
        uint8_t r = sl.r;
        uint8_t g = sl.g;
        uint8_t b = sl.b;
        if (m_model.getGameState() == GameState::Menu
            || m_model.getGameState() == GameState::Paused) {
            // Dim everything if the menu is displayed
            r *= 0.4;
            g *= 0.4;
            b *= 0.4;
        }
        m_graphicsAdapter.drawLine(
            x0r * scale + xOffset,
            y0r * scale + yOffset,
            x1r * scale + xOffset,
            y1r * scale + yOffset,
            sl.lineThickness,
            r,
            g,
            b);
    }
}

void View::drawStaticShape(const GameShape& shape) const
{
    // Note that static images' coordinates' origin is TOP LEFT OF THE SCREEN
    double scale = m_graphicsAdapter.getWindoWidth() / 480.0;

    for (const auto& sl : shape.getVec()) {
        m_graphicsAdapter.drawLine(
            (sl.x0 + shape.getPosX()) * scale,
            (sl.y0 + shape.getPosY()) * scale,
            (sl.x1 + shape.getPosX()) * scale,
            (sl.y1 + shape.getPosY()) * scale,
            sl.lineThickness,
            sl.r,
            sl.g,
            sl.b);
    }
}

} // namespace amaze
} // namespace marengo
