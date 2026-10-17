#ifndef BULLET_H
#define BULLET_H

#include <memory>
#include <vector>

#include "CollisionObject.h"
#include "GameContext.h"

//-----------------------------------------------------------------------
/**
 * @brief A laser bolt fired by either the player or an enemy.
 *
 * Each bullet stores the segment it travelled during the last frame and uses
 * that swept segment as its collision shape, so a fast shot cannot tunnel
 * through a small enemy between frames. When the bullet hits a valid target it
 * stops drawing as a bolt and instead leaves a single dot at the point of
 * impact for one frame before the engine removes it.
 *
 * A bullet holds only a weak reference to whatever fired it: if the owner dies
 * the bullet dies with it, and the reference never keeps the owner alive.
 */
//-----------------------------------------------------------------------
class Bullet : public CMPUT350::CollisionObject
{
public:
    /**
     * @brief Creates a bullet that moves a fixed amount each frame.
     * @param location Starting world position.
     * @param heading Per-frame displacement (the bullet's velocity vector).
     * @param player true for a player bullet, false for an enemy bullet.
     * @param parent The object that fired this bullet, held weakly.
     */
    Bullet(CMPUT350::Point2D location, CMPUT350::Vector2D heading, bool player, std::weak_ptr<CMPUT350::GameObject> parent);

    /// @brief True if this bullet was fired by the player.
    bool IsPlayerBullet() const;

    // GameObject Functions
    void Initialize(CMPUT350::GameContext* context) override;
    void Update(CMPUT350::GameContext* context) override;
    void LateUpdate(CMPUT350::GameContext* context) override;
    bool IsAlive() const override;
    void Kill() override;

    // Graphics Object Functions
    void RenderBackground(CMPUT350::GameContext* context) override;
    void RenderForeground(CMPUT350::GameContext* context) override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;

    // Collision Object Functions
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj) override;
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj, const CMPUT350::Point2D& collisionPoint) override;
    const CMPUT350::Rect& GetBounds() override;
    const std::vector<CMPUT350::Shape>& GetShapes() override;
private:
    CMPUT350::Point2D mLocation;         // Current world position.
    CMPUT350::Point2D mPreviousLocation; // Position at the start of this frame.
    CMPUT350::Vector2D mHeading;         // Per-frame displacement (velocity).

    bool mPlayer;                        // Which side fired this bullet.
    std::weak_ptr<CMPUT350::GameObject> mParent; // The firing object.
    bool mAlive;                         // Removed by the engine once false.
    bool mDestroyed;                     // True after hitting a target.
    bool mHasCollisionPoint;             // Whether mCollisionPoint is valid.
    CMPUT350::Point2D mCollisionPoint;   // Local impact point used when destroyed.
    CMPUT350::Rect mBounds;              // World-space broad-phase box.
    std::vector<CMPUT350::Shape> mShapes; // Single swept line segment.
};
#endif // BULLET_H
