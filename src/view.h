#pragma once
#include "gamemodel.h"

// This class acts as the MVC "view" component - its function is to
// update the display according to the state held within the model.
//
// Note that the actual size of the window on the screen is determined
// by the graphics adapter. The view treats this as a window onto the Level
// arena with a constant area (equivalent to 480x300 arena units), whatever
// the window's size or shape.
//
// The "view" also includes other output, e.g. sounds.

namespace marengo {
namespace amaze {

class View {
public:
    View(GameModel& model, IGraphicsAdapter& gm);
    void draw(); // this is called once per displayed frame
    void rotateAndDrawShape(const GameShape& shape) const;
    void playSounds(); // this is called once per simulation tick
    void stopSounds();

private:
    GameModel& m_model;
    IGraphicsAdapter& m_graphicsAdapter;
};

} // namespace amaze
} // namespace marengo
