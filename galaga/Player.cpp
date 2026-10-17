#include <algorithm>
#include <cmath>

#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "GraphicsObject.h"
#include "NotificationManager.h"

namespace {

// Half the ship's width, used to keep the hull inside the window.
constexpr float kHalfWidth = 20.0f;
// A double ship is wider because the captured fighter docks alongside.
constexpr float kDoubleHalfWidth = 34.0f;
// Horizontal gap between the two hulls of a double ship.
constexpr float kDoubleOffset = 14.0f;
// Row the ship returns to after being freed from a capture.
constexpr float kHomeRowY = 900.0f;
// Pixels per frame the ship falls while dropping back into play.
constexpr float kDropSpeed = 14.0f;
// Radians per frame the captured ship spins.
constexpr float kCaptureSpin = 0.15f;

} // namespace

//-----------------------------------------------------------------------
/**
 * @brief Creates the player ship.
 *
 * The hitbox is built from two local rectangles (a wide base and a narrow
 * nose) so the collision shape roughly follows the drawn ship. The ship starts
 * under player control as a single fighter.
 * @param loc Centre of the ship in world coordinates.
 */
//-----------------------------------------------------------------------
Player::Player(CMPUT350::Point2D loc)
    : mLoc(loc),
      mBounds(loc - kHalfWidth, 2.0f * kHalfWidth, 2.0f * kHalfWidth),
      mAlive(true),
      mSpeed(7.0f),
      mMoveLeft(false),
      mMoveRight(false),
      mFireRequested(false),
      mNotificationManager(nullptr),
      mState(State::Active),
      mDoubleShip(false),
      mRotation(0.0f),
      mDropTargetY(kHomeRowY)
{
    RefreshBounds();
}

//-----------------------------------------------------------------------
/**
 * @brief Caches the notification manager and subscribes to input events.
 *
 * The engine sets CurrObject to this player before calling Initialize, so the
 * weak self-reference lets the ship register for the movement, shooting, and
 * (for the keyboard setup screen) notification names the Galaga manager
 * registered with the engine.
 * @param context The engine's per-frame context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Initialize(CMPUT350::GameContext* context)
{
    // The engine sets CurrObject to this player before calling Initialize.
    const std::weak_ptr<CMPUT350::GameObject> self = context->CurrObject;

    if (self.expired() || context->mNotificationManager == nullptr)
    {
        return;
    }

    mNotificationManager = context->mNotificationManager;

    // Subscribe the player to movement and shooting actions.
    auto* notifications = context->mNotificationManager;
    notifications->Register(self, kMoveLeftDown);
    notifications->Register(self, kMoveLeftUp);
    notifications->Register(self, kMoveRightDown);
    notifications->Register(self, kMoveRightUp);
    notifications->Register(self, kShoot);
}

//-----------------------------------------------------------------------
/**
 * @brief Rebuilds local hitboxes and their rotated world-space bounds.
 *
 * A double ship has one hitbox per fighter. Each local rectangle is transformed
 * by the current rotation before the broad-phase bounds are calculated, so the
 * engine cannot reject a collision that the narrow phase would accept.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::RefreshBounds()
{
    mShapes.clear();
    const auto addFighterShapes = [this](float offset)
    {
        mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(
            offset - kHalfWidth,
            -kHalfWidth,
            2.0f * kHalfWidth,
            2.0f * kHalfWidth
        )));
        mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(
            offset - 5.0f,
            -25.0f,
            10.0f,
            15.0f
        )));
    };

    if (mDoubleShip)
    {
        addFighterShapes(-kDoubleOffset);
        addFighterShapes(kDoubleOffset);
    }
    else
    {
        addFighterShapes(0.0f);
    }

    bool firstPoint = true;
    CMPUT350::Point2D minimum;
    CMPUT350::Point2D maximum;
    for (const auto& shape : mShapes)
    {
        for (const auto& localPoint : shape.shape.rect.Points())
        {
            const CMPUT350::Point2D worldPoint =
                localPoint.Rotate(mRotation) + mLoc;
            if (firstPoint)
            {
                minimum = worldPoint;
                maximum = worldPoint;
                firstPoint = false;
            }
            else
            {
                minimum.x = std::min(minimum.x, worldPoint.x);
                minimum.y = std::min(minimum.y, worldPoint.y);
                maximum.x = std::max(maximum.x, worldPoint.x);
                maximum.y = std::max(maximum.y, worldPoint.y);
            }
        }
    }

    mBounds = CMPUT350::Rect(minimum, maximum);
}

//-----------------------------------------------------------------------
/**
 * @brief Moves the ship, or follows its captor while captured.
 *
 * While Active the direction is read from flags set by notifications and
 * applied here, so movement is frame-rate independent: the ship advances a
 * fixed distance each frame rather than once per key event. Holding both keys
 * cancels out. The position is clamped so the hull stays fully inside the
 * playfield, and the bounding box is re-derived afterwards.
 *
 * While Captured the ship is positioned above the capturing enemy and spins,
 * and while Dropping it falls back to the bottom row, both ignoring input.
 *
 * @param context Supplies the window width and the engine used to spawn shots.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Update(CMPUT350::GameContext* context)
{
    if (!mAlive)
    {
        return;
    }

    // Captured: follow the enemy that holds us and spin in place.
    if (mState == State::Captured)
    {
        const auto captorBase = mCaptor.lock();
        const auto captor = std::dynamic_pointer_cast<CMPUT350::GraphicsObject>(captorBase);

        if (captor == nullptr || !captorBase->IsAlive())
        {
            // The captor vanished without a clean release; drop as a normal ship.
            Release(false);
        }
        else
        {
            mLoc = captor->GetLocation() + CMPUT350::Point2D(0.0f, -44.0f);
            mRotation += kCaptureSpin;
            RefreshBounds();
            return;
        }
    }

    // Dropping: fall straight back to the home row, ignoring input.
    if (mState == State::Dropping)
    {
        mLoc.y += kDropSpeed;
        if (mLoc.y >= mDropTargetY)
        {
            mLoc.y = mDropTargetY;
            mState = State::Active;
        }
        mRotation = 0.0f;
        RefreshBounds();
        return;
    }

    float direction = 0.0f;
    if (mMoveLeft)
    {
        direction -= 1.0f;
    }
    if (mMoveRight)
    {
        direction += 1.0f;
    }

    if (direction != 0.0f)
    {
        const float halfWidth = mDoubleShip ? kDoubleHalfWidth : kHalfWidth;
        float minX = halfWidth;
        float maxX = 768.0f - halfWidth;

        if (context != nullptr && context->ScreenContext != nullptr)
        {
            maxX = static_cast<float>(context->ScreenContext->GetWindowWidth()) - halfWidth;
        }

        mLoc.x = std::clamp(mLoc.x + direction * mSpeed, minX, maxX);
    }

    RefreshBounds();

    if (mFireRequested)
    {
        mFireRequested = false;
        Fire(context);
    }
}

void Player::LateUpdate(CMPUT350::GameContext* context)
{
    (void)context;
}

void Player::RenderBackground(CMPUT350::GameContext* context)
{
    (void)context;
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the ship as a blocky Galaga-style sprite.
 *
 * The rectangles are relative to the ship's own origin, since the engine
 * applies mLoc through the DrawContext before this runs. When the ship is a
 * double fighter the same sprite is drawn twice, offset to either side.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::RenderForeground(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->ScreenContext == nullptr || !mAlive)
    {
        return;
    }

    auto* screen = context->ScreenContext;

    // Body of one fighter, centred on the given local origin.
    const auto drawFighter = [screen](float originX)
    {
        screen->DrawRect(CMPUT350::Rect(originX - 5.0f, -15.0f, 10.0f, 20.0f), CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 10.0f, -5.0f, 5.0f, 15.0f), CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX + 5.0f, -5.0f, 5.0f, 15.0f), CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 15.0f, 10.0f, 30.0f, 10.0f), CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 20.0f, 5.0f, 5.0f, 10.0f), CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX + 15.0f, 5.0f, 5.0f, 10.0f), CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 20.0f, 0.0f, 5.0f, 5.0f), CMPUT350::Colors::red);
        screen->DrawRect(CMPUT350::Rect(originX + 15.0f, 0.0f, 5.0f, 5.0f), CMPUT350::Colors::red);
        screen->DrawRect(CMPUT350::Rect(originX - 5.0f, -20.0f, 10.0f, 5.0f), CMPUT350::Colors::red);
        screen->DrawRect(CMPUT350::Rect(originX - 5.0f, 5.0f, 10.0f, 5.0f), CMPUT350::Colors::blue);
    };

    if (mDoubleShip)
    {
        drawFighter(-kDoubleOffset);
        drawFighter(kDoubleOffset);
    }
    else
    {
        drawFighter(0.0f);
    }
}

CMPUT350::Point2D Player::GetLocation() const
{
    return mLoc;
}

float Player::GetRotation() const
{
    return mRotation;
}

//-----------------------------------------------------------------------
/**
 * @brief Handles the ship touching an enemy, enemy fire, or friendly fire.
 *
 * An Active ship is only harmed by enemies and enemy bullets; its own bullets
 * pass through. A Captured ship can instead be destroyed by the player's own
 * fire, and is otherwise safe while it is held or dropping back into play.
 * @param obj The object that overlapped the ship.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    if (!mAlive || obj == nullptr)
    {
        return;
    }

    const auto bullet = std::dynamic_pointer_cast<Bullet>(obj);

    if (mState == State::Captured)
    {
        // A captured fighter is destroyed if the player shoots it.
        if (bullet != nullptr && bullet->IsPlayerBullet())
        {
            Kill();
        }
        return;
    }

    if (mState != State::Active)
    {
        // Dropping ships are invulnerable while they return to the row.
        return;
    }

    const bool hitByEnemy = std::dynamic_pointer_cast<Enemy>(obj) != nullptr;
    const bool hitByEnemyBullet = bullet != nullptr && !bullet->IsPlayerBullet();

    if (hitByEnemy || hitByEnemyBullet)
    {
        Kill();
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Kills the ship and announces it to the Galaga manager.
 *
 * The engine removes the ship once IsAlive returns false. The manager decides
 * whether the ship has a spare life and when to respawn it.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Kill()
{
    if (!mAlive)
    {
        return;
    }

    mAlive = false;
    if (mNotificationManager != nullptr)
    {
        mNotificationManager->Notify(kDestroyedNotification);
    }
}

bool Player::IsAlive() const
{
    return mAlive;
}

//-----------------------------------------------------------------------
/**
 * @brief Spawns bullets from the ship's cannons.
 *
 * A single fighter may have two shots in flight; a double fighter fires two
 * bullets per volley and may have four in flight. Expired slots are discarded
 * first, and no bullet is fired when every slot is full.
 * @param context Supplies the engine used to add the bullets.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Fire(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->mEngineView == nullptr)
    {
        return;
    }

    // Forget bullets that have already left the scene.
    mBullets.erase(
        std::remove_if(
            mBullets.begin(),
            mBullets.end(),
            [](const std::weak_ptr<Bullet>& bullet) { return bullet.expired(); }
        ),
        mBullets.end()
    );

    const std::size_t maxSlots = mDoubleShip ? 4u : 2u;
    const std::vector<float> muzzles = mDoubleShip
        ? std::vector<float>{-kDoubleOffset, kDoubleOffset}
        : std::vector<float>{0.0f};

    for (float offset : muzzles)
    {
        if (mBullets.size() >= maxSlots)
        {
            break;
        }

        auto bullet = std::make_shared<Bullet>(
            mLoc + CMPUT350::Point2D(offset, -18.0f),
            CMPUT350::Point2D(0.0f, -40.0f),
            true,
            context->CurrObject
        );

        mBullets.push_back(bullet);
        context->mEngineView->AddGameObject(bullet);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Updates the held-direction flags and the fire request.
 *
 * Movement keys simply set or clear a direction flag, and the shoot key sets a
 * one-shot flag that Update consumes. Notifications are ignored while the ship
 * is captured or dropping so it cannot be steered during those states.
 * @param notification The notification name delivered by the manager.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::ReceiveNotification(const std::string& notification)
{
    if (notification == kMoveLeftDown)
    {
        mMoveLeft = true;
    }
    else if (notification == kMoveLeftUp)
    {
        mMoveLeft = false;
    }
    else if (notification == kMoveRightDown)
    {
        mMoveRight = true;
    }
    else if (notification == kMoveRightUp)
    {
        mMoveRight = false;
    }
    else if (notification == kShoot)
    {
        mFireRequested = true;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Starts a capture: the ship stops responding and follows the enemy.
 * @param captor The enemy taking the ship; held weakly.
 * @return None. Has no effect unless the ship is currently Active.
 */
//-----------------------------------------------------------------------
void Player::CaptureBy(std::weak_ptr<CMPUT350::GameObject> captor)
{
    if (mState != State::Active || !mAlive)
    {
        return;
    }

    mState = State::Captured;
    mCaptor = captor;
    mRotation = 0.0f;

    // Drop any held input so the ship does not drift when freed.
    mMoveLeft = false;
    mMoveRight = false;
    mFireRequested = false;
}

//-----------------------------------------------------------------------
/**
 * @brief Frees the ship, which then falls back to the bottom row.
 * @param asDoubleShip true if the freed fighter should dock and become a double.
 * @return None. Has no effect unless the ship is currently Captured.
 */
//-----------------------------------------------------------------------
void Player::Release(bool asDoubleShip)
{
    if (mState != State::Captured)
    {
        return;
    }

    mCaptor.reset();
    mDoubleShip = asDoubleShip;
    mState = State::Dropping;
    mRotation = 0.0f;
    mDropTargetY = kHomeRowY;
}

//-----------------------------------------------------------------------
/**
 * @brief Reports whether an enemy is currently holding the ship.
 * @return true while captured.
 */
//-----------------------------------------------------------------------
bool Player::IsCaptured() const
{
    return mState == State::Captured;
}

//-----------------------------------------------------------------------
/**
 * @brief Reports whether the ship is under player control.
 * @return true while active.
 */
//-----------------------------------------------------------------------
bool Player::IsActive() const
{
    return mState == State::Active;
}

//-----------------------------------------------------------------------
/**
 * @brief Reports whether the ship has been upgraded to a double fighter.
 * @return true once the captured fighter has been freed.
 */
//-----------------------------------------------------------------------
bool Player::IsDoubleShip() const
{
    return mDoubleShip;
}

//-----------------------------------------------------------------------
/**
 * @brief Returns the ship's world-space bounding box for broad-phase checks.
 * @return The current bounding box.
 */
//-----------------------------------------------------------------------
const CMPUT350::Rect& Player::GetBounds()
{
    return mBounds;
}

//-----------------------------------------------------------------------
/**
 * @brief Returns the ship's local collision shapes.
 *
 * The shapes are in the ship's local space; the engine offsets them by
 * GetLocation() and applies the (usually zero) rotation when testing.
 * @return The list of rectangles making up the ship's hitbox.
 */
//-----------------------------------------------------------------------
const std::vector<CMPUT350::Shape>& Player::GetShapes()
{
    return mShapes;
}
