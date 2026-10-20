#ifndef GRAPHICS_OBJECT_H
#define GRAPHICS_OBJECT_H

#include "GameObject.h"
#include "MathUtil.h"

namespace CMPUT350 {

class GameContext;

class GraphicsObject : public GameObject {
public:
    // Draw through the borrowed context; default implementations draw nothing.
    virtual void RenderBackground(GameContext *context);
    virtual void RenderForeground(GameContext *context);
    // Return the screen-space rotation center and clockwise rotation in radians.
    virtual Point2D GetLocation() const = 0;
    virtual float GetRotation() const = 0;
};

}  // namespace CMPUT350

#endif
