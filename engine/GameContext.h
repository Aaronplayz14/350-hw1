#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H

#include "DrawContext.h"
#include "EngineView.h"
#include "GameObject.h"

namespace CMPUT350 {

class GameContext {
public:
    EngineView *mEngineView;
    DrawContext *ScreenContext;
};

}  // namespace CMPUT350

#endif  // GAMECONTEXT_H
