#pragma once
#include "gamemodel.h"

// This class acts as the MVC "view" component - its function is to
// update the display according to the state held within the model.
//
// Note that the actual size of the window on the screen is determined
// by the graphics adapter. The view simply treats this as a (roughly)
// 20% x 15% window onto the Level arena.
//
// The "view" also includes other output, e.g. sounds.

namespace marengo {
namespace amaze {

class View {
public:
    View(GameModel& model, IGraphicsAdapter& gm);
    void Update(); // this is called once per game loop iteration
    void RotateAndDrawShape(const GameShape& shape) const;
    void DrawStaticShape(const GameShape& shape) const;
    void PlaySounds();
    void stopSounds();

private:
    GameModel& m_Model;
    IGraphicsAdapter& m_GraphicsAdapter;
};

} // namespace amaze
} // namespace marengo
