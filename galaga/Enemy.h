#ifndef ENEMY_H
#define ENEMY_H
#include <memory>
#include <vector>

#include "Bezier.h"
#include "CollisionObject.h"
#include "GameContext.h"

namespace CMPUT350 {
    class NotificationManager;
}

//-----------------------------------------------------------------------
/**
 * @brief Base class for every Galaga enemy.
 *
 * This class holds the behaviour shared by all enemy types: following a cubic
 * Bezier trajectory into formation, hovering in formation, returning to
 * formation after an attack, taking damage, and reporting its own destruction
 * through the notification manager. A subclass only has to supply its Attack
 * behaviour, which is why Enemy is abstract.
 *
 * An enemy is drawn in its own local space (centred on the origin) and moved
 * by the engine using the location returned from GetLocation(), so subclasses
 * never need to know their world position when rendering.
 */
//-----------------------------------------------------------------------
class Enemy : public CMPUT350::CollisionObject
{
public:
    /// @brief Notification sent to the game manager when any enemy is destroyed.
    static constexpr const char* kDestroyedNotification = "EnemyDestroyed";

    /**
     * @brief Builds an enemy at its formation slot with an entry trajectory.
     *
     * If entryPath is a valid (3n + 1 point) cubic Bezier, the enemy starts at
     * the first control point and follows the curve to the formation location.
     * Otherwise it is placed directly in formation.
     *
     * @param loc Centre of the enemy's formation slot.
     * @param entryPath Cubic Bezier control points used to fly into formation.
     * @param health Number of player hits the enemy can absorb.
     * @param pointValue Score the enemy is worth before any formation bonus.
     */
    Enemy(
        CMPUT350::Point2D loc,
        const std::vector<CMPUT350::Point2D>& entryPath,
        int health = 1,
        int pointValue = 100
    );

    /// @brief Starts an attack against a target; false if one cannot begin.
    virtual bool Attack(CMPUT350::GameContext* context, const CMPUT350::Point2D& target) = 0;

    // GameObject Functions
    void Initialize(CMPUT350::GameContext* context) override;
    void Update(CMPUT350::GameContext* context) override;
    void LateUpdate(CMPUT350::GameContext* context) override;
    bool IsAlive() const override;
    void Kill() override;

    // Graphics Object Functions
    void RenderBackground(CMPUT350::GameContext* context) override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;

    // Collision Object Functions
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj) override;
    const CMPUT350::Rect& GetBounds() override;
    const std::vector<CMPUT350::Shape>& GetShapes() override;

    /// @brief True while the enemy is holding its formation slot and not moving.
    bool IsInFormation() const;

    /// @brief Remaining hit points; the enemy dies at or below zero.
    int GetHealth() const;

    /// @brief Hit points the enemy began with, used to detect damage.
    int GetMaxHealth() const;

    /// @brief Base score this enemy is worth before any formation bonus.
    int GetPointValue() const;

    /// @brief True if the enemy was in formation at the instant it died.
    ///
    /// The game manager uses this together with GetPointValue() to award more
    /// points for an enemy caught outside its formation.
    bool WasInFormationWhenKilled() const;

protected:
    /**
     * @brief Starts following a Bezier path as an attack run.
     *
     * Dispatches the curve to the shared trajectory machinery and tags it as an
     * attack, so that finishing the path automatically starts the return leg.
     *
     * @param path Cubic Bezier control points for the attack.
     * @param speed Distance covered per frame along the path.
     * @return true if the attack started, false if the enemy was not in
     *         formation or the path was invalid.
     */
    bool BeginAttackPath(
        const std::vector<CMPUT350::Point2D>& path,
        float speed
    );

    /**
     * @brief Spawns an enemy bullet aimed at a target.
     *
     * The bullet is parented to this enemy so that it disappears if the enemy
     * dies. The direction is normalised before the speed is applied.
     *
     * @param context Used to reach the engine for object creation.
     * @param target World point to aim at (normally the player).
     * @param speed Bullet speed in pixels per frame.
     * @return None. The bullet is queued with the engine.
     */
    void FireAtTarget(
        CMPUT350::GameContext* context,
        const CMPUT350::Point2D& target,
        float speed
    );

private:
    /// @brief Why a trajectory is being followed, which decides what happens on arrival.
    enum class TrajectoryPurpose {
        EnterFormation,   // Flying in from off-screen.
        Attack,           // Running an attack curve toward the player.
        ReturnToFormation // Heading back to the formation slot after an attack.
    };

    /// @brief Builds the distance-sampled representation of a Bezier path.
    bool StartTrajectory(
        const std::vector<CMPUT350::Point2D>& path,
        float speed,
        TrajectoryPurpose purpose
    );

    /// @brief Advances along the current trajectory and updates rotation.
    void AdvanceTrajectory();

    /// @brief Starts a Bezier curve from the current spot back to formation.
    bool BeginReturnToFormation(float speed);

    /// @brief Applies one hit of damage, killing the enemy at zero health.
    void ApplyDamage();

    CMPUT350::Point2D mLoc;              // Current world position (rotation centre).
    CMPUT350::Point2D mFormationLocation; // Where the enemy belongs in formation.
    CMPUT350::Rect mBounds;              // World-space broad-phase bounding box.
    float mRotation;                     // Facing angle along the trajectory, radians.
    bool mAlive;
    int mHealth;
    int mMaxHealth;
    int mPointValue;
    bool mInFormation;                   // True once the entry path is complete.
    bool mMovingOnPath;                  // True while following a Bezier trajectory.
    bool mWasInFormationWhenKilled;      // Formation state captured in Kill().
    float mDistanceAlongTrajectory;      // Arc length covered on the current path.
    float mTrajectoryLength;             // Total arc length of the current path.
    float mMovementSpeed;                // Pixels travelled per frame.
    std::vector<CMPUT350::Shape> mShapes; // Single circle collision shape.
    std::unique_ptr<CMPUT350::Bezier> mTrajectory;
    std::vector<float> mTrajectoryParameterSamples; // t values for each sample.
    std::vector<float> mTrajectoryDistanceSamples;  // Arc length for each sample.
    CMPUT350::Point2D mTrajectoryEnd;    // Last control point of the active path.
    TrajectoryPurpose mTrajectoryPurpose;
    CMPUT350::NotificationManager* mNotificationManager; // Set during Initialize.
    std::weak_ptr<CMPUT350::GameObject> mSelf; // Parent reference for spawned bullets.
};

//-----------------------------------------------------------------------
/**
 * @brief Fast, fragile enemy drawn as a cyan triangle.
 *
 * Scouts have a single hit point, score the least, and dash toward the player
 * on a short, steep curve before returning.
 */
//-----------------------------------------------------------------------
class EnemyScout final : public Enemy
{
public:
    EnemyScout(CMPUT350::Point2D loc, const std::vector<CMPUT350::Point2D>& entryPath);

    /// @brief Dives steeply at the target, firing once as the attack begins.
    bool Attack(CMPUT350::GameContext* context, const CMPUT350::Point2D& target) override;
    void RenderForeground(CMPUT350::GameContext* context) override;
};

//-----------------------------------------------------------------------
/**
 * @brief Swooping enemy drawn as a magenta diamond.
 *
 * Weavers share the scout's single hit point but score more, and swing wide
 * around the player on a looping curve.
 */
//-----------------------------------------------------------------------
class EnemyWeaver final : public Enemy
{
public:
    EnemyWeaver(CMPUT350::Point2D loc, const std::vector<CMPUT350::Point2D>& entryPath);

    /// @brief Sweeps in a wide arc around the target, firing on the way in.
    bool Attack(CMPUT350::GameContext* context, const CMPUT350::Point2D& target) override;
    void RenderForeground(CMPUT350::GameContext* context) override;
};

//-----------------------------------------------------------------------
/**
 * @brief Tough enemy drawn as a red square inside a white frame.
 *
 * Armored enemies take two player hits and score the most. They turn yellow
 * after the first hit and charge in on a broad, heavy curve.
 */
//-----------------------------------------------------------------------
class EnemyArmored final : public Enemy
{
public:
    EnemyArmored(CMPUT350::Point2D loc, const std::vector<CMPUT350::Point2D>& entryPath);

    /// @brief Charges across the screen toward the target, firing once.
    bool Attack(CMPUT350::GameContext* context, const CMPUT350::Point2D& target) override;
    void RenderForeground(CMPUT350::GameContext* context) override;
};

#endif
