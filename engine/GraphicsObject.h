#ifndef GRAPHICS_OBJECT_H
#define GRAPHICS_OBJECT_H

#include "GameObject.h"
#include "MathUtil.h"

namespace CMPUT350 {

class GameContext;

class GraphicsObject : public GameObject {
public:
    /// @brief Draws behind other objects; default draws nothing.
    virtual void RenderBackground(GameContext *context);
    /// @brief Draws in front of other objects; default draws nothing.
    virtual void RenderForeground(GameContext *context);
    // Center of object with respect to rotation
	virtual Point2D GetLocation() const;
	// Return the rotation of the object in radians
	virtual float GetRotation() const;
};

}  // namespace CMPUT350

#endif // GRAPHICS_OBJECT_H
