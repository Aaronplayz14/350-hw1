#include "GraphicsObject.h"

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Draws behind other objects; default is to draw nothing.
 * @param context Provides the drawing contexts.
 * @return None.
 */
//-----------------------------------------------------------------------
void GraphicsObject::RenderBackground(GameContext *context) { return; }

//-----------------------------------------------------------------------
/**
 * @brief Draws in front of other objects; default is to draw nothing.
 * @param context Provides the drawing contexts.
 * @return None.
 */
//-----------------------------------------------------------------------
void GraphicsObject::RenderForeground(GameContext *context) { return; }

}  // namespace CMPUT350
