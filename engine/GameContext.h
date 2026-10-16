#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H
#include <memory>

#include "DrawContext.h"
#include "EngineView.h"
#include "GameObject.h"

namespace CMPUT350 {

class NotificationManager;

//-----------------------------------------------------------------------
/**
 * @brief The bundle of services handed to a game object each callback.
 *
 * Every engine callback receives one of these, giving the object a view of the
 * engine, the two drawing contexts, a weak reference to itself, the
 * notification manager, and the global frame counter.
 *
 * The two drawing contexts differ only in transforms: ScreenContext carries
 * the current object's offset and rotation and is used for gameplay, while
 * GUIContext is untransformed and is meant for HUD drawing from RenderUI.
 * Objects should only draw to the GUI context from RenderUI.
 */
//-----------------------------------------------------------------------
class GameContext {
public:
    EngineView *EngineContext;                      // Engine access (add objects, iterate).
    DrawContext *ScreenContext;                     // Transformed context for gameplay drawing.
    DrawContext *GUIContext;                        // Untransformed context for the HUD.
    std::weak_ptr<GameObject> CurrObject;           // The object being updated/drawn.
    NotificationManager *Notifications;             // Shared notification manager.
    int currentFrame = 0;                           // Global frame counter / clock.
};

}  // namespace CMPUT350

#endif  // GAMECONTEXT_H
