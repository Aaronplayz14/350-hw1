#include "GameObject.h"

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Called once when the object first enters the scene.
 *
 * The default implementation does nothing, so an object that needs no setup
 * need not override this.
 * @param context Provides access to the engine and drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameObject::Initialize(GameContext *context) { return; }

//-----------------------------------------------------------------------
/**
 * @brief Called once per frame to advance the object's simulation.
 *
 * The default implementation does nothing.
 * @param context Provides access to the engine and drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameObject::Update(GameContext *context) { return; }

//-----------------------------------------------------------------------
/**
 * @brief Called once per frame after all collisions have been resolved.
 *
 * Use this instead of Update for anything that must observe the final state of
 * the frame, such as reacting to a collision that happened during Update.
 * @param context Provides access to the engine and drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameObject::LateUpdate(GameContext *context) { return; }

//-----------------------------------------------------------------------
/**
 * @brief Called each frame for drawing objects that render on their own layer.
 *
 * The default implementation does nothing.
 * @param contextrender Provides access to the engine and drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameObject::RenderUI(GameContext *contextrender) { return; }

void GameObject::ReceiveNotification(const std::string& notification)
{
    // Default: this object does not respond to notifications.
}
//-----------------------------------------------------------------------
/**
 * @brief Reports whether the object should survive to the next frame.
 *
 * The engine removes any object returning false before the next frame begins,
 * so returning false is how an object retires itself. The default is true,
 * meaning an object that never overrides this is never removed.
 * @return true while the object is alive, false once it should be removed.
 */
//-----------------------------------------------------------------------
bool GameObject::IsAlive() const { return true; }

//-----------------------------------------------------------------------
/**
 * @brief Marks the object for removal at the start of the next frame.
 *
 * Does not destroy the object directly; the engine drops it from its list
 * once IsAlive starts returning false.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameObject::Kill() {}
}  // namespace CMPUT350
