#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H
#include <memory>

#include "DrawContext.h"
#include "EngineView.h"
#include "GameObject.h"

namespace CMPUT350 {

class NotificationManager;

class GameContext {
public:
    EngineView *EngineContext;
    DrawContext *ScreenContext;
    DrawContext *GUIContext;
    std::weak_ptr<GameObject> CurrObject;
    NotificationManager *NotificationManager;
    int currentFrame = 0;
};

}  // namespace CMPUT350

#endif  // GAMECONTEXT_H