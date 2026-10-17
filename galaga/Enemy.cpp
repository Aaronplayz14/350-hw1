#include <algorithm>
#include <cmath>
#include <utility>

#include "Enemy.h"
#include "Bullet.h"
#include "NotificationManager.h"
#include "Player.h"

namespace {

constexpr float kEnemyRadius = 15.0f;
constexpr float kEntrySpeed = 3.0f;
constexpr int kSamplesPerBezierSegment = 40;
// How close the player must be for a capture run to grab it. This is slightly
// larger than the combined collision radii so the grab happens before the two
// objects are registered as colliding.
constexpr float kCaptureRange = 42.0f;

// The Bezier implementation requires 3*n + 1 control points.
bool IsValidBezierPath(const std::vector<CMPUT350::Point2D>& points)
{
    return points.size() >= 4 && (points.size() - 1) % 3 == 0;
}

} // namespace
//-----------------------------------------------------------------------
/**
 * @brief Creates an enemy with a formation position, entry path, health, and score value.
 *
 * When a valid entry path is supplied the enemy is placed at the path's first
 * control point and immediately starts following the curve; otherwise it is
 * dropped straight into its formation slot. Health and score are clamped to
 * sane minimums so a caller cannot create a zero-health or negative-score
 * enemy.
 *
 * @param formationLocation Centre of the enemy's target formation slot.
 * @param entryPath Cubic Bezier control points used to enter formation.
 * @param health Number of player hits the enemy can withstand.
 * @param pointValue Score value assigned to this enemy type.
 */
//-----------------------------------------------------------------------
Enemy::Enemy(
    CMPUT350::Point2D formationLocation,
    const std::vector<CMPUT350::Point2D>& entryPath,
    int health,
    int pointValue
):
    mLoc(IsValidBezierPath(entryPath) ? entryPath.front() : formationLocation),
    mFormationLocation(formationLocation),
    mRotation(0.0f),
    mBounds(mLoc - kEnemyRadius, 2.0f * kEnemyRadius, 2.0f * kEnemyRadius),
    mAlive(true),
    mHealth(std::max(1, health)),
    mMaxHealth(std::max(1, health)),
    mPointValue(std::max(0, pointValue)),
    mInFormation(!IsValidBezierPath(entryPath)),
    mMovingOnPath(false),
    mWasInFormationWhenKilled(false),
    mScoreClaimed(false),
    mDistanceAlongTrajectory(0.0f),
    mTrajectoryLength(0.0f),
    mMovementSpeed(kEntrySpeed),
    mTrajectoryEnd(formationLocation),
    mTrajectoryPurpose(TrajectoryPurpose::EnterFormation),
    mNotificationManager(nullptr),
    mCaptureAttempt(false)
{
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Circle(0.0f, 0.0f, kEnemyRadius)));
    if (IsValidBezierPath(entryPath))
    {
        StartTrajectory(entryPath, kEntrySpeed, TrajectoryPurpose::EnterFormation);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Caches the services the enemy needs later in the frame.
 *
 * The notification manager is used to announce this enemy's destruction, and
 * the weak self-reference is passed to any bullets the enemy fires so they can
 * be cleaned up when it dies.
 *
 * @param context The engine's per-frame context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::Initialize(CMPUT350::GameContext* context)
{
    if (context != nullptr)
    {
        mNotificationManager = context->mNotificationManager;
        mSelf = context->CurrObject;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Advances the enemy along its trajectory or lets it hover in formation.
 *
 * Movement follows the current Bezier path when one is active; otherwise an
 * idle enemy oscillates gently around its formation slot. The hover is derived
 * from the global frame counter (offset by the slot's x position) so a whole
 * formation breathes with different phases but deterministically, without any
 * per-enemy clock. The world-space bounding box is refreshed at the end.
 *
 * @param context Supplies the frame counter used for the formation hover.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::Update(CMPUT350::GameContext* context)
{
    if (!mAlive)
    {
        return;
    }

    if (mMovingOnPath)
    {
        AdvanceTrajectory();
    }

    // Formation movement is shared by all enemy types.
    if (mInFormation && !mMovingOnPath)
    {
        const float frame = context != nullptr
            ? static_cast<float>(context->currentFrame)
            : 0.0f;

        // Use position as a deterministic phase offset so ships do not
        // all hover in exactly the same way.
        const float phase = mFormationLocation.x * 0.017f;
        const float time = frame * 0.035f + phase;

        mLoc.x = mFormationLocation.x + std::sin(time) * 12.0f;
        mLoc.y = mFormationLocation.y + std::sin(time * 0.5f) * 3.0f;

        mRotation = 0.0f;
    }

    // A capture run takes the player as soon as it comes within range. Doing
    // this in Update means the ship is already flagged as captured before the
    // engine resolves collisions, so the ram check below cannot kill it.
    if (mCaptureAttempt && mCapturedPlayer.expired())
    {
        TryCapture(context);
    }

    mBounds = CMPUT350::Rect(mLoc - kEnemyRadius, 2.0f * kEnemyRadius, 2.0f * kEnemyRadius);
}

void Enemy::LateUpdate(CMPUT350::GameContext* context)
{
    (void)context;
}

void Enemy::RenderBackground(CMPUT350::GameContext* context)
{
    (void)context;
}

CMPUT350::Point2D Enemy::GetLocation() const
{
    return mLoc;
}

float Enemy::GetRotation() const
{
    return mRotation;
}

//-----------------------------------------------------------------------
/**
 * @brief Reacts to touching the player or a player bullet.
 *
 * An enemy may only be harmed by the player or the player's bullets. Ramming
 * the player destroys the enemy outright, while a player bullet removes one
 * hit point (enough to kill a scout or weaver but only damage an armored
 * enemy). Enemy bullets and other enemies are ignored so a formation cannot
 * shoot or bump itself out of the sky.
 * @param obj The object whose bounds overlapped this enemy.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    if (!mAlive)
    {
        return;
    }
    if (std::dynamic_pointer_cast<Player>(obj) != nullptr)
    {
        // While running a capture (or holding a ship) the enemy must not ram
        // the player: the grab is handled in Update, and once the ship is
        // captured it is this enemy's prize rather than a collision.
        if (IsCapturing())
        {
            return;
        }
        Kill();
        return;
    }
    const auto bullet = std::dynamic_pointer_cast<Bullet>(obj);
    if (bullet != nullptr && bullet->IsPlayerBullet())
    {
        ApplyDamage();
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Marks the enemy dead and announces it to the game manager.
 *
 * The formation state is captured first so that the manager can award the
 * "caught out of formation" bonus even though the enemy is already dead by the
 * time the notification is delivered.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::Kill()
{
    if (!mAlive)
    {
        return;
    }

    mWasInFormationWhenKilled = IsInFormation();
    mAlive = false;

    // If this enemy was holding the player's ship, the capture is broken and
    // the freed fighter returns as a double ship (extra credit).
    if (auto player = mCapturedPlayer.lock())
    {
        player->Release(true);
    }
    mCapturedPlayer.reset();

    if (mNotificationManager != nullptr)
    {
        mNotificationManager->Notify(kDestroyedNotification);
    }
}

bool Enemy::IsAlive() const
{
    return mAlive;
}

const CMPUT350::Rect& Enemy::GetBounds()
{
    return mBounds;
}

const std::vector<CMPUT350::Shape>& Enemy::GetShapes()
{
    return mShapes;
}

bool Enemy::IsInFormation() const
{
    return mInFormation && !mMovingOnPath;
}

int Enemy::GetHealth() const
{
    return mHealth;
}

int Enemy::GetMaxHealth() const
{
    return mMaxHealth;
}

int Enemy::GetPointValue() const
{
    return mPointValue;
}

bool Enemy::WasInFormationWhenKilled() const
{
    return mWasInFormationWhenKilled;
}

//-----------------------------------------------------------------------
/**
 * @brief Returns this kill's score once, then reports zero.
 *
 * EnemyDestroyed carries no payload, so the manager finds the just-killed enemy
 * through the engine and calls this to read its value. Claiming the score here
 * keeps a single death from being counted twice if several enemies die in the
 * same frame. Out-of-formation kills are worth double.
 *
 * @return The points this enemy is worth, or 0 if already claimed/alive.
 */
//-----------------------------------------------------------------------
int Enemy::TakeScore()
{
    if (mScoreClaimed || mAlive)
    {
        return 0;
    }

    mScoreClaimed = true;

    int value = mPointValue;

    if (!mWasInFormationWhenKilled)
    {
        value *= 2;
    }

    return value;
}

//-----------------------------------------------------------------------
/**
 * @brief Flags the next attack as a capture run (extra credit).
 *
 * The manager calls this immediately before Attack so the enemy knows to aim at
 * the player and to grab the ship on contact rather than ramming it.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::BeginCapture()
{
    mCaptureAttempt = true;
}

//-----------------------------------------------------------------------
/**
 * @brief Reports whether this enemy is on a capture run or holding a ship.
 * @return true if a capture has been requested or a ship is being held.
 */
//-----------------------------------------------------------------------
bool Enemy::IsCaptureAttempt() const
{
    return mCaptureAttempt;
}

//-----------------------------------------------------------------------
/**
 * @brief Reports whether the capture logic should override normal ramming.
 * @return true while attempting to capture or already holding a ship.
 */
//-----------------------------------------------------------------------
bool Enemy::IsCapturing() const
{
    return mCaptureAttempt || !mCapturedPlayer.expired();
}

//-----------------------------------------------------------------------
/**
 * @brief Grabs the player's ship when it is close enough (extra credit).
 *
 * The engine is searched for the active player (the ship can only be captured
 * while under player control). If the ship is within the capture radius it is
 * told to follow this enemy, and the enemy remembers it weakly so that freeing
 * or destroying it later can be handled cleanly. The search stops after the
 * first successful grab.
 * @param context Supplies the engine used to find the player.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::TryCapture(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->mEngineView == nullptr)
    {
        return;
    }

    for (auto it = context->mEngineView->begin();
         it != context->mEngineView->end();
         ++it)
    {
        auto player = std::dynamic_pointer_cast<Player>(*it);

        if (player == nullptr || !player->IsAlive() || !player->IsActive())
        {
            continue;
        }

        if (player->GetLocation().Distance(mLoc) <= kCaptureRange)
        {
            player->CaptureBy(mSelf);
            mCapturedPlayer = player;

            // The grab succeeded, so no further capture attempts are needed.
            mCaptureAttempt = false;
            break;
        }
    }
}
//-----------------------------------------------------------------------
/**
 * @brief Begins an attack by following a Bezier curve.
 *
 * Only an enemy that is sitting in its formation slot may start an attack, so
 * a manager that tries to launch an already-flying enemy simply gets false.
 *
 * @param path Cubic Bezier control points for the attack run.
 * @param speed Distance covered per frame.
 * @return true if the attack started, false otherwise.
 */
//-----------------------------------------------------------------------
bool Enemy::BeginAttackPath(const std::vector<CMPUT350::Point2D>& path, float speed)
{
    if (!mAlive || !IsInFormation())
    {
        return false;
    }

    return StartTrajectory(path, speed, TrajectoryPurpose::Attack);
}

//-----------------------------------------------------------------------
/**
 * @brief Precomputes the arc-length table for a Bezier path and starts it.
 *
 * Cubic Beziers do not travel at a constant speed for evenly spaced t values,
 * so the curve is sampled into many points and the cumulative straight-line
 * distance to each sample is stored. AdvanceTrajectory then walks the curve by
 * distance, which makes the enemy move at a nearly constant rate regardless of
 * how the control points are spaced.
 *
 * @param path Cubic Bezier control points (3n + 1 of them).
 * @param speed Distance to travel per frame.
 * @param purpose What the path represents, which decides behaviour on arrival.
 * @return true if a usable trajectory was built, false for a degenerate path.
 */
//-----------------------------------------------------------------------
bool Enemy::StartTrajectory(
    const std::vector<CMPUT350::Point2D>& path,
    float speed,
    TrajectoryPurpose purpose
)
{
    if (!IsValidBezierPath(path))
    {
        return false;
    }

    auto trajectory = std::make_unique<CMPUT350::Bezier>(path);

    const std::size_t segmentCount = (path.size() - 1) / 3;
    const std::size_t sampleCount =
        segmentCount * kSamplesPerBezierSegment;

    std::vector<float> parameters;
    std::vector<float> distances;

    parameters.reserve(sampleCount + 1);
    distances.reserve(sampleCount + 1);

    float totalDistance = 0.0f;
    CMPUT350::Point2D previousPoint = trajectory->GetPoint(0.0f);

    for (std::size_t i = 0; i <= sampleCount; ++i)
    {
        const float t =
            static_cast<float>(i) /
            static_cast<float>(kSamplesPerBezierSegment);

        const CMPUT350::Point2D point = trajectory->GetPoint(t);

        if (i > 0)
        {
            totalDistance += previousPoint.Distance(point);
        }

        parameters.push_back(t);
        distances.push_back(totalDistance);

        previousPoint = point;
    }

    if (totalDistance <= 0.001f)
    {
        return false;
    }

    mTrajectory = std::move(trajectory);
    mTrajectoryParameterSamples = std::move(parameters);
    mTrajectoryDistanceSamples = std::move(distances);

    mDistanceAlongTrajectory = 0.0f;
    mTrajectoryLength = totalDistance;
    mMovementSpeed = std::max(speed, 0.1f);
    mTrajectoryEnd = path.back();
    mTrajectoryPurpose = purpose;

    mLoc = path.front();
    mMovingOnPath = true;
    mInFormation = false;

    return true;
}

//-----------------------------------------------------------------------
/**
 * @brief Moves the enemy along the current trajectory using arc length.
 *
 * The stored distance is advanced by the enemy's speed and then converted back
 * into a Bezier parameter by interpolating between the two nearest distance
 * samples. When the path is exhausted, what happens next depends on why the
 * enemy was flying: an attack schedules a return to formation, while entry and
 * return curves simply drop the enemy back into its slot.
 *
 * @return None. May start a new trajectory or clear the current one.
 */
//-----------------------------------------------------------------------
void Enemy::AdvanceTrajectory()
{
    if (!mTrajectory || !mMovingOnPath)
    {
        return;
    }

    mDistanceAlongTrajectory = std::min(
        mDistanceAlongTrajectory + mMovementSpeed,
        mTrajectoryLength
    );

    // Finish the active curve when its length has been traversed.
    if (mDistanceAlongTrajectory >= mTrajectoryLength)
    {
        const TrajectoryPurpose completedPurpose = mTrajectoryPurpose;
        const float returnSpeed = mMovementSpeed;

        mLoc = mTrajectoryEnd;

        mTrajectory.reset();
        mTrajectoryParameterSamples.clear();
        mTrajectoryDistanceSamples.clear();

        mDistanceAlongTrajectory = 0.0f;
        mTrajectoryLength = 0.0f;
        mMovingOnPath = false;

        if (completedPurpose == TrajectoryPurpose::Attack)
        {
            // The attack is over but the enemy is not in formation yet, so send
            // it back to its slot on a fresh curve.
            mInFormation = false;

            BeginReturnToFormation(returnSpeed);
        }
        else
        {
            // Entry and return paths finish at the formation location.
            mLoc = mFormationLocation;
            mRotation = 0.0f;
            mInFormation = true;
        }

        return;
    }

    // Find the two samples surrounding the target distance.
    const auto upper = std::lower_bound(
        mTrajectoryDistanceSamples.begin(),
        mTrajectoryDistanceSamples.end(),
        mDistanceAlongTrajectory
    );

    const std::size_t highIndex = static_cast<std::size_t>(
        upper - mTrajectoryDistanceSamples.begin()
    );

    if (highIndex == 0)
    {
        mLoc = mTrajectory->GetPoint(
            mTrajectoryParameterSamples.front()
        );
        return;
    }

    const std::size_t lowIndex = highIndex - 1;

    const float lowDistance =
        mTrajectoryDistanceSamples[lowIndex];

    const float highDistance =
        mTrajectoryDistanceSamples[highIndex];

    const float distanceSpan = highDistance - lowDistance;

    float fraction = 0.0f;

    if (distanceSpan > 0.0001f)
    {
        fraction =
            (mDistanceAlongTrajectory - lowDistance) /
            distanceSpan;
    }

    const float lowT = mTrajectoryParameterSamples[lowIndex];
    const float highT = mTrajectoryParameterSamples[highIndex];

    const float t = lowT + (highT - lowT) * fraction;

    mLoc = mTrajectory->GetPoint(t);

    // Face along the direction of travel.
    const CMPUT350::Point2D slope = mTrajectory->GetSlope(t);

    if (slope.Distance(CMPUT350::Point2D(0.0f, 0.0f)) > 0.0001f)
    {
        mRotation = std::atan2(slope.y, slope.x);
    }
}
//-----------------------------------------------------------------------
/**
 * @brief Builds a curve from the enemy's current position back to its slot.
 *
 * The control points arc the return path upward (negative y) so a returning
 * enemy lifts away from the player before settling back into formation instead
 * of flying straight through the play area.
 *
 * @param speed Distance to travel per frame on the way back.
 * @return true if the return trajectory started, false if the enemy was
 *         already in formation, already moving, or dead.
 */
//-----------------------------------------------------------------------
bool Enemy::BeginReturnToFormation(float speed)
{
    if (!mAlive || mInFormation || mMovingOnPath)
    {
        return false;
    }

    const CMPUT350::Point2D start = mLoc;
    const CMPUT350::Point2D end = mFormationLocation;

    // A cubic curve that arcs back toward the formation.
    const std::vector<CMPUT350::Point2D> returnPath = {
        start,
        start + CMPUT350::Point2D(0.0f, -80.0f),
        end + CMPUT350::Point2D(0.0f, 80.0f),
        end
    };

    return StartTrajectory(
        returnPath,
        speed,
        TrajectoryPurpose::ReturnToFormation
    );
}

//-----------------------------------------------------------------------
/**
 * @brief Fires an enemy bullet toward a target.
 *
 * The bullet is marked as an enemy bullet and parented to this enemy, so it is
 * removed automatically when the enemy dies. The shot is skipped if the engine
 * is unavailable, this enemy is already gone, or the target is on top of it.
 *
 * @param context Provides the engine used to add the bullet.
 * @param target World position to aim at (normally the player).
 * @param speed Bullet speed in pixels per frame.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::FireAtTarget(
    CMPUT350::GameContext* context,
    const CMPUT350::Point2D& target,
    float speed
)
{
    if (context == nullptr || context->mEngineView == nullptr || mSelf.expired())
    {
        return;
    }

    CMPUT350::Vector2D direction = target - mLoc;

    if (direction.Distance(CMPUT350::Point2D(0.0f, 0.0f)) <= 0.001f)
    {
        return;
    }

    direction.Normalize();
    direction *= std::max(speed, 0.1f);

    auto bullet = std::make_shared<Bullet>(
        mLoc,
        direction,
        false,
        mSelf
    );

    context->mEngineView->AddGameObject(bullet);
}

//-----------------------------------------------------------------------
/**
 * @brief Removes one hit point and destroys the enemy if none are left.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::ApplyDamage()
{
    if (!mAlive)
    {
        return;
    }

    --mHealth;

    if (mHealth <= 0)
    {
        Kill();
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Creates a scout: one hit point, worth 100 points.
 * @param loc Formation slot centre.
 * @param entryPath Bezier control points for flying into formation.
 * @return None.
 */
//-----------------------------------------------------------------------
EnemyScout::EnemyScout(
    CMPUT350::Point2D loc,
    const std::vector<CMPUT350::Point2D>& entryPath
): Enemy(loc, entryPath, 1, 100)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Sends the scout on a steep dive toward the target.
 *
 * The attack curve drops nearly straight down and stops just past the target,
 * then the scout fires a single fast shot before returning to formation.
 * @param context Passed through to the shot helper.
 * @param target World position to dive at and shoot at.
 * @return true if the dive started, false if the scout was not in formation.
 */
//-----------------------------------------------------------------------
bool EnemyScout::Attack(
    CMPUT350::GameContext* context,
    const CMPUT350::Point2D& target
)
{
    const CMPUT350::Point2D start = GetLocation();
    const CMPUT350::Point2D end = target + CMPUT350::Point2D(0.0f, 110.0f);
    const std::vector<CMPUT350::Point2D> path = {
        start,
        start + CMPUT350::Point2D(-35.0f, 100.0f),
        end + CMPUT350::Point2D(35.0f, -100.0f),
        end
    };

    if (!BeginAttackPath(path, 5.0f))
    {
        return false;
    }
    FireAtTarget(context, target, 6.0f);
    return true;
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the scout as a cyan triangle, yellow after taking a hit.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void EnemyScout::RenderForeground(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->ScreenContext == nullptr || !IsAlive())
    {
        return;
    }

    const auto color = GetHealth() < GetMaxHealth()
        ? CMPUT350::Colors::yellow
        : CMPUT350::Colors::cyan;
    context->ScreenContext->DrawLine({-15.0f, -10.0f}, {0.0f, 15.0f}, 3.0f, color);
    context->ScreenContext->DrawLine({0.0f, 15.0f}, {15.0f, -10.0f}, 3.0f, color);
    context->ScreenContext->DrawLine({15.0f, -10.0f}, {-15.0f, -10.0f}, 3.0f, color);
}

//-----------------------------------------------------------------------
/**
 * @brief Creates a weaver: one hit point, worth 150 points.
 * @param loc Formation slot centre.
 * @param entryPath Bezier control points for flying into formation.
 * @return None.
 */
//-----------------------------------------------------------------------
EnemyWeaver::EnemyWeaver(
    CMPUT350::Point2D loc,
    const std::vector<CMPUT350::Point2D>& entryPath
): Enemy(loc, entryPath, 1, 150)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Sends the weaver on a wide, looping pass around the target.
 *
 * The control points swing the enemy far to the right of the player and then
 * back, so the path reads very differently from the scout's straight dive.
 * @param context Passed through to the shot helper.
 * @param target World position the loop is built around.
 * @return true if the pass started, false if the weaver was not in formation.
 */
//-----------------------------------------------------------------------
bool EnemyWeaver::Attack(
    CMPUT350::GameContext* context,
    const CMPUT350::Point2D& target
)
{
    const CMPUT350::Point2D start = GetLocation();
    const CMPUT350::Point2D end = target + CMPUT350::Point2D(90.0f, 130.0f);
    const std::vector<CMPUT350::Point2D> path = {
        start,
        start + CMPUT350::Point2D(130.0f, 80.0f),
        end + CMPUT350::Point2D(-130.0f, -120.0f),
        end
    };

    if (!BeginAttackPath(path, 3.5f))
    {
        return false;
    }
    FireAtTarget(context, target, 4.5f);
    return true;
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the weaver as a magenta diamond, yellow after taking a hit.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void EnemyWeaver::RenderForeground(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->ScreenContext == nullptr || !IsAlive())
    {
        return;
    }

    const auto color = GetHealth() < GetMaxHealth()
        ? CMPUT350::Colors::yellow
        : CMPUT350::Colors::magenta;
    context->ScreenContext->DrawLine({-15.0f, 0.0f}, {0.0f, -12.0f}, 4.0f, color);
    context->ScreenContext->DrawLine({0.0f, -12.0f}, {15.0f, 0.0f}, 4.0f, color);
    context->ScreenContext->DrawLine({15.0f, 0.0f}, {0.0f, 12.0f}, 4.0f, color);
    context->ScreenContext->DrawLine({0.0f, 12.0f}, {-15.0f, 0.0f}, 4.0f, color);
}

//-----------------------------------------------------------------------
/**
 * @brief Creates an armored enemy: two hit points, worth 250 points.
 * @param loc Formation slot centre.
 * @param entryPath Bezier control points for flying into formation.
 * @return None.
 */
//-----------------------------------------------------------------------
EnemyArmored::EnemyArmored(
    CMPUT350::Point2D loc,
    const std::vector<CMPUT350::Point2D>& entryPath
): Enemy(loc, entryPath, 2, 250)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Sends the armored enemy on a broad, heavy charging curve.
 *
 * The wider control points and slower speed make this the most deliberate of
 * the three attacks. It takes a single shot as it begins.
 * @param context Passed through to the shot helper.
 * @param target World position to charge at and shoot at.
 * @return true if the charge started, false if the enemy was not in formation.
 */
//-----------------------------------------------------------------------
bool EnemyArmored::Attack(
    CMPUT350::GameContext* context,
    const CMPUT350::Point2D& target
)
{
    const CMPUT350::Point2D start = GetLocation();
    const bool capturing = IsCaptureAttempt();

    // A capture run ends right on the player so the grab range is reached; a
    // normal charge instead overshoots below and to the side of the target.
    const CMPUT350::Point2D end = capturing
        ? target
        : target + CMPUT350::Point2D(-100.0f, 150.0f);
    const std::vector<CMPUT350::Point2D> path = {
        start,
        start + CMPUT350::Point2D(-120.0f, 120.0f),
        end + CMPUT350::Point2D(120.0f, -180.0f),
        end
    };

    if (!BeginAttackPath(path, 2.5f))
    {
        return false;
    }

    // A capturing enemy holds its fire so it cannot destroy the ship it is
    // trying to steal.
    if (!capturing)
    {
        FireAtTarget(context, target, 5.0f);
    }
    return true;
}
//-----------------------------------------------------------------------
/**
 * @brief Draws the armored enemy as a red square in a white frame.
 *
 * The square turns yellow once the enemy has lost its first hit point, which
 * is the visual cue that it is one hit from dying.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void EnemyArmored::RenderForeground(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->ScreenContext == nullptr || !IsAlive())
    {
        return;
    }

    const auto color = GetHealth() < GetMaxHealth()
        ? CMPUT350::Colors::yellow
        : CMPUT350::Colors::red;
    context->ScreenContext->DrawRect(CMPUT350::Rect(-12.0f, -12.0f, 24.0f, 24.0f), color);
    context->ScreenContext->FrameRect(CMPUT350::Rect(-15.0f, -15.0f, 30.0f, 30.0f), 2.0f, CMPUT350::Colors::white);
}