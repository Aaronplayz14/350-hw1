#include "GraphicsObject.h"
#include "MathUtil.h"

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Draws behind other objects; default is to draw nothing.
 * @param contextrender Provides the drawing contexts.
 * @return None.
 */
//-----------------------------------------------------------------------
void GraphicsObject::RenderBackground(GameContext *contextrender) { return; }

//-----------------------------------------------------------------------
/**
 * @brief Draws in front of other objects; default is to draw nothing.
 * @param contextrender Provides the drawing contexts.
 * @return None.
 */
//-----------------------------------------------------------------------
void GraphicsObject::RenderForeground(GameContext *contextrender) { return; }

//-----------------------------------------------------------------------
/**
 * @brief Origin of the object for drawing, and the point it rotates around.
 * @return The object's location; the origin by default.
 */
//-----------------------------------------------------------------------
Point2D GraphicsObject::GetLocation() const { return Point2D();}

//-----------------------------------------------------------------------
/**
 * @brief Rotation applied to the object when drawing and colliding.
 * @return The rotation in radians; zero by default.
 */
//-----------------------------------------------------------------------
float GraphicsObject::GetRotation() const { return 0.0; }

}  // namespace CMPUT350
