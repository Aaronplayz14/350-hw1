#ifndef GRAPHICS_OBJECT_H
#define GRAPHICS_OBJECT_H

#include "GameObject.h"

namespace CMPUT350 {

class GameContext;

class GraphicsObject : public GameObject {
public:
    virtual void RenderBackground(GameContext *context);
    virtual void RenderForeground(GameContext *context);
};

}  // namespace CMPUT350

#endif
