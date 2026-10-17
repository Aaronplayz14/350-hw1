#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include <memory>

#include "CollisionObject.h"
#include "Bullet.h"

namespace CMPUT350 { class NotificationManager; }

//-----------------------------------------------------------------------
/**
 * @brief The player's ship.
 *
 * The ship normally moves horizontally along the bottom of the playfield. It
 * never changes its own location directly from key input: instead the Galaga
 * manager registers the key bindings with the engine, the engine turns those
 * keys into notifications, the ship records which directions are held, and
 * Update applies the movement. That keeps movement smooth and frame-based
 * rather than one step per key event.
 *
 * The ship draws itself in a fixed local space no matter where it is; the
 * engine applies GetLocation() as the drawing offset, so moving the ship is
 * only a matter of returning a new location.
 *
 * The ship can also be captured by an enemy (extra credit). While captured it
 * follows the capturing enemy and spins, ignoring input. It can be destroyed
 * by the player's own fire, or freed as a double ship if the capturing enemy
 * is destroyed.
 */
//-----------------------------------------------------------------------
class Player : public CMPUT350::CollisionObject
{
public:
    /// @brief Notification names the player listens for; the Galaga manager
    ///        registers these against the engine's keys.
    static constexpr const char* kMoveLeftDown = "player.move.left.down";
    static constexpr const char* kMoveLeftUp = "player.move.left.up";
    static constexpr const char* kMoveRightDown = "player.move.right.down";
    static constexpr const char* kMoveRightUp = "player.move.right.up";
    static constexpr const char* kShoot = "player.shoot";

    /// @brief Notification the player sends when it is destroyed.
    static constexpr const char* kDestroyedNotification = "PlayerDestroyed";

    /// @brief The stage of the ship's life, which decides how Update behaves.
    enum class State
    {
        Active,   ///< Under player control along the bottom row.
        Captured, ///< Held above the formation, spinning, ignoring input.
        Dropping  ///< Falling back to the bottom row after being freed.
    };

    /// @brief Creates the ship centred on the given world position.
    Player(CMPUT350::Point2D loc);

    // GameObject Functions
    void Initialize(CMPUT350::GameContext* context) override;
    void Update(CMPUT350::GameContext* context) override;
    void LateUpdate(CMPUT350::GameContext* context) override;
    void ReceiveNotification(const std::string& notification) override;

    bool IsAlive() const override;
    void Kill() override;

    // Graphics Object Functions
    void RenderBackground(CMPUT350::GameContext* context) override;
    void RenderForeground(CMPUT350::GameContext* context) override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;

    // Collision Object Functions
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj) override;
    const CMPUT350::Rect& GetBounds() override;
    const std::vector<CMPUT350::Shape>& GetShapes() override;

    // Capture-specific behaviour (extra credit).
    /// @brief Hands control of the ship to a capturing enemy.
    void CaptureBy(std::weak_ptr<CMPUT350::GameObject> captor);
    /// @brief Frees a captured ship so it falls back to the bottom row.
    /// @param asDoubleShip true if it should return as a double fighter.
    void Release(bool asDoubleShip);

    /// @brief True while an enemy is holding the ship.
    bool IsCaptured() const;
    /// @brief True while the ship is under player control.
    bool IsActive() const;
    /// @brief True once the ship has been upgraded to a double fighter.
    bool IsDoubleShip() const;

private:
    /// @brief Fires one bullet per available slot.
    void Fire(CMPUT350::GameContext* context);
    /// @brief Recomputes the world bounding box from the current location.
    void RefreshBounds();

    CMPUT350::Point2D mLoc;             // Centre of the ship in world space.
    CMPUT350::Rect mBounds;             // World-space broad-phase box.
    bool mAlive;
    float mSpeed;                       // Pixels moved per frame while a key is held.
    bool mMoveLeft;                     // Set by notifications while left is held.
    bool mMoveRight;                    // Set by notifications while right is held.
    bool mFireRequested;                // One-shot flag set by the shoot notification.
    std::vector<CMPUT350::Shape> mShapes; // Local hitbox shapes.
    CMPUT350::NotificationManager* mNotificationManager; // Set during Initialize.

    // Weak on purpose: the engine owns the bullets, and an expired slot is
    // what tells us we are allowed to fire again.
    std::vector<std::weak_ptr<Bullet>> mBullets;

    State mState;                       // Active, Captured, or Dropping.
    std::weak_ptr<CMPUT350::GameObject> mCaptor; // Enemy holding this ship.
    bool mDoubleShip;                   // True after being freed by a capture.
    float mRotation;                    // Spin while captured.
    float mDropTargetY;                 // Row the ship falls back to.
};

#endif
