#ifndef GRAPHICS_OBJECT_H
#define GRAPHICS_OBJECT_H

#include "GameObject.h"
#include "MathUtil.h"

namespace CMPUT350 {

class GameContext;

class GraphicsObject : public GameObject {
public:
    virtual void RenderBackground(GameContext *context);
    virtual void RenderForeground(GameContext *context);
    // Center of object with respect to rotation
	virtual Point2D GetLocation() const;
	// Return the rotation of the object in radians
	virtual float GetRotation() const;
};

}  // namespace CMPUT350

#endif
