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

//-----------------------------------------------------------------------
/**
 * @brief Called once per key press for one-shot inputs such as firing.
 *
 * Only fire on the press, not while the key is held, so a single press does
 * not produce a burst of shots. For continuous input such as movement, use
 * HandleKeyState instead.
 * @param context Provides access to the engine and drawing context.
 * @param key The character that was pressed.
 * @return true if the key was consumed, false to let it pass unhandled.
 */
//-----------------------------------------------------------------------
bool GameObject::HandleKeyEvent(GameContext *context, char key) { return false; }

//-----------------------------------------------------------------------
/**
 * @brief Called on press and again on release for continuously held keys.
 *
 * Store the pressed state and act on it during Update rather than moving the
 * object here, which is what makes movement speed independent of frame rate.
 * @param context Provides access to the engine and drawing context.
 * @param key The movement character, 'a' for left or 'd' for right.
 * @param pressed true when the key goes down, false when it is released.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameObject::HandleKeyState(GameContext *context, char key, bool pressed) { return; }

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
