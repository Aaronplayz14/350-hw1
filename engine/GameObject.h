#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>

namespace CMPUT350 {

class GameContext;

//-----------------------------------------------------------------------
/**
 * @brief Base class for everything the engine can hold in its scene.
 *
 * The engine drives each object through a fixed sequence of virtual callbacks:
 * Initialize once, then Update, collision handling, LateUpdate, and the render
 * passes every frame. Only IsAlive is consulted to decide whether the object
 * survives, so an object retires itself by returning false (or calling Kill).
 *
 * Key input is not part of this interface; objects learn about input through
 * notifications delivered to ReceiveNotification.
 */
//-----------------------------------------------------------------------
class GameObject {
public:
    virtual ~GameObject() = default;

    /// @brief Called once when the object first enters the scene.
    virtual void Initialize(GameContext *context);
    /// @brief Called every frame to advance the object's simulation.
    virtual void Update(GameContext *context);
    /// @brief Called every frame after collisions have been resolved.
    virtual void LateUpdate(GameContext *context);
    /// @brief Called to draw HUD-style content that ignores object transforms.
    virtual void RenderUI(GameContext *context);

    /// @brief Called when a notification this object listens for is sent.
    virtual void ReceiveNotification(const std::string& notification);

    /// @brief Whether the object should remain in the scene.
    virtual bool IsAlive() const;
    /// @brief Requests removal at the start of the next frame.
    virtual void Kill();
};

}  // namespace CMPUT350

#endif
