#include <algorithm>

#include "Player.h"
#include "Bullet.h"
#include "Keyboard.h"
#include "NotificationManager.h"
namespace {

// Names of the notifications the player listens for. The engine is told to
// send these when the matching keys are pressed or released.
constexpr char kMoveLeftDown[] = "player.move.left.down";
constexpr char kMoveLeftUp[] = "player.move.left.up";

constexpr char kMoveRightDown[] = "player.move.right.down";
constexpr char kMoveRightUp[] = "player.move.right.up";

constexpr char kShoot[] = "player.shoot";

// Half the ship's width, used to keep the hull inside the window.
constexpr float kHalfWidth = 20.0f;

} // namespace

//-----------------------------------------------------------------------
/**
 * @brief Creates the player ship.
 *
 * The hitbox is a 40x40 box centred on the location, built from two local
 * rectangles (a wide base and a narrow nose) so the collision shape roughly
 * follows the drawn ship. Rotation is always zero, so the box never needs to
 * be re-derived.
 * @param loc Centre of the ship in world coordinates.
 */
//-----------------------------------------------------------------------
Player::Player(CMPUT350::Point2D loc):mLoc(loc), mBounds(loc - kHalfWidth, 2.0f * kHalfWidth, 2.0f * kHalfWidth), mAlive(true), mSpeed(7.0f),
    mMoveLeft(false), mMoveRight(false), mFireRequested(false)
{
    // These hitboxes are local to mLoc; the engine adds GetLocation() when it
    // tests them, so the world position must not be baked in here.
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(-20.0f, 0.0f, 40.0f, 20.0f)));
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(-5.0f, -20.0f, 10.0f, 20.0f)));
}

//-----------------------------------------------------------------------
/**
 * @brief Subscribes the ship to its movement and shooting notifications.
 *
 * Two things happen here: the player registers itself as a listener for the
 * notification names it cares about, and the engine is told which physical
 * keys should emit those notifications. A and Left both map to the left
 * action, D and Right both map to the right action, and Space emits a shot.
 *
 * Note: the assignment eventually expects the Galaga game object to install
 * the key bindings and the player to only register. Until Galaga exists the
 * player performs both steps so the ship is playable on its own.
 *
 * @param context The engine's per-frame context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Initialize(CMPUT350::GameContext* context)
{
    if (context == nullptr) {
        return;
    }

    // The engine sets CurrObject to this player before calling Initialize.
    const std::weak_ptr<CMPUT350::GameObject> self = context->CurrObject;

    if (self.expired() || context->Notifications == nullptr
        || context->EngineContext == nullptr) {
        return;
    }

    auto* notifications = context->Notifications;
    auto* engine = context->EngineContext;

    // Subscribe the player to movement and shooting actions.
    notifications->Register(self, kMoveLeftDown);
    notifications->Register(self, kMoveLeftUp);
    notifications->Register(self, kMoveRightDown);
    notifications->Register(self, kMoveRightUp);
    notifications->Register(self, kShoot);

    // A and Left Arrow begin moving left.
    engine->InstallKeyDownNotification(CMPUT350::Key::A, kMoveLeftDown);
    engine->InstallKeyUpNotification(CMPUT350::Key::A, kMoveLeftUp);

    engine->InstallKeyDownNotification(CMPUT350::Key::Left, kMoveLeftDown);
    engine->InstallKeyUpNotification(CMPUT350::Key::Left, kMoveLeftUp);

    // D and Right Arrow begin moving right.
    engine->InstallKeyDownNotification(CMPUT350::Key::D, kMoveRightDown);
    engine->InstallKeyUpNotification(CMPUT350::Key::D, kMoveRightUp);

    engine->InstallKeyDownNotification(CMPUT350::Key::Right, kMoveRightDown);
    engine->InstallKeyUpNotification(CMPUT350::Key::Right, kMoveRightUp);

    // Space requests one shot.
    engine->InstallKeyDownNotification(CMPUT350::Key::Space, kShoot);
}

//-----------------------------------------------------------------------
/**
 * @brief Moves the ship horizontally and fires when requested.
 *
 * The direction is read from flags set by notifications and applied here, so
 * movement is frame-rate independent: the ship advances a fixed distance each
 * frame rather than once per key event. Holding both keys cancels out.
 *
 * The position is clamped so the hull stays fully inside the playfield, using
 * the window width when it is available, and the bounding box origin is
 * re-derived afterwards to keep the centre and bounds in sync. Firing only
 * succeeds while a free bullet slot is available, which caps the ship at two
 * shots on screen at once.
 *
 * @param context Supplies the window width and the engine used to spawn shots.
 * @return None. Updates mLoc, mBounds, and may queue a bullet.
 */
//-----------------------------------------------------------------------
void Player::Update(CMPUT350::GameContext* context)
{
    float direction = 0.0f;
    if (mMoveLeft) {
        direction -= 1.0f;
    }
    if (mMoveRight) {
        direction += 1.0f;
    }

    if (direction != 0.0f) {
        // Clamp so the 40px-wide hull stays inside the window. The right edge
        // is taken from the window when possible so the ship still works if
        // the window is resized; 768 is the fallback playfield width.
        float windowWidth = 768.0f;
        if (context != nullptr && context->ScreenContext != nullptr) {
            windowWidth = static_cast<float>(context->ScreenContext->GetWindowWidth());
        }

        mLoc.x = std::clamp(
            mLoc.x + direction * mSpeed,
            kHalfWidth,
            windowWidth - kHalfWidth
        );
        mBounds.topLeft.x = mLoc.x - kHalfWidth;
    }
    if (mFireRequested) {
        mFireRequested = false;

        // Without the engine there is nowhere to send the bullet.
        if (context == nullptr || context->EngineContext == nullptr) {
            return;
        }

        if (b1.expired()) {
            auto bullet = std::make_shared<Bullet>(
                CMPUT350::Point2D(mLoc),
                CMPUT350::Point2D(0.0f, -40.0f),
                true,
                context->CurrObject
            );
            b1 = bullet;
            context->EngineContext->AddGameObject(bullet);
        } else if (b2.expired()) {
            auto bullet = std::make_shared<Bullet>(
                CMPUT350::Point2D(mLoc),
                CMPUT350::Point2D(0.0f, -40.0f),
                true,
                context->CurrObject
            );
            b2 = bullet;
            context->EngineContext->AddGameObject(bullet);
        }
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
 * applies mLoc through the DrawContext before this runs. White blocks form
 * the hull and wings, red marks the cockpit and engine, and a blue block is
 * the base. Collision stays a simple box rather than matching these blocks
 * exactly, which is the usual trade-off for a ship this small.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::RenderForeground(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->ScreenContext == nullptr) {
        return;
    }

    auto* screen = context->ScreenContext;

    screen->DrawRect(CMPUT350::Rect(-5.0f, -15.0f, 10.0f, 20.0f), CMPUT350::Colors::white);
    screen->DrawRect(CMPUT350::Rect(-10.0f, -5.0f, 5.0f, 15.0f), CMPUT350::Colors::white);
    screen->DrawRect(CMPUT350::Rect(5.0f, -5.0f, 5.0f, 15.0f), CMPUT350::Colors::white);
    screen->DrawRect(CMPUT350::Rect(-15.0f, 10.0f, 30.0f, 10.0f), CMPUT350::Colors::white);
    screen->DrawRect(CMPUT350::Rect(-20.0f, 5.0f, 5.0f, 10.0f), CMPUT350::Colors::white);
    screen->DrawRect(CMPUT350::Rect(15.0f, 5.0f, 5.0f, 10.0f), CMPUT350::Colors::white);
    screen->DrawRect(CMPUT350::Rect(-20.0f, 0.0f, 5.0f, 5.0f), CMPUT350::Colors::red);
    screen->DrawRect(CMPUT350::Rect(15.0f, 0.0f, 5.0f, 5.0f), CMPUT350::Colors::red);
    screen->DrawRect(CMPUT350::Rect(-5.0f, -20.0f, 10.0f, 5.0f), CMPUT350::Colors::red);
    screen->DrawRect(CMPUT350::Rect(-5.0f, 5.0f, 10.0f, 5.0f), CMPUT350::Colors::blue);
}

CMPUT350::Point2D Player::GetLocation() const
{
    return mLoc;
}

float Player::GetRotation() const
{
    return 0.0;
}

//-----------------------------------------------------------------------
/**
 * @brief Reaction to being hit by an enemy or an enemy bullet.
 *
 * Left intentionally empty for now: losing a life, playing a death animation,
 * and respawning all belong to the Galaga game manager, which is not written
 * yet. The enemy bullets still destroy themselves on contact, so the ship
 * simply cannot be killed until that manager exists.
 * @param obj The object that overlapped the ship.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    (void)obj;
}

//-----------------------------------------------------------------------
/**
 * @brief Kills the ship immediately.
 *
 * The engine removes the ship once IsAlive returns false. Respawning is the
 * game manager's responsibility.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Kill()
{
    mAlive = false;
}

bool Player::IsAlive() const
{
    return mAlive;
}

//-----------------------------------------------------------------------
/**
 * @brief Updates the held-direction flags and the fire request.
 *
 * Movement keys simply set or clear a direction flag, and the shoot key sets a
 * one-shot flag that Update consumes. Because these only record intent, a key
 * held across several frames keeps moving the ship every frame.
 * @param notification The notification name delivered by the manager.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::ReceiveNotification(const std::string& notification)
{
    if (notification == kMoveLeftDown) {
        mMoveLeft = true;
    }
    else if (notification == kMoveLeftUp) {
        mMoveLeft = false;
    }
    else if (notification == kMoveRightDown) {
        mMoveRight = true;
    }
    else if (notification == kMoveRightUp) {
        mMoveRight = false;
    }
    else if (notification == kShoot) {
        mFireRequested = true;
    }
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
 * GetLocation() and applies the (always zero) rotation when testing.
 * @return The list of rectangles making up the ship's hitbox.
 */
//-----------------------------------------------------------------------
const std::vector<CMPUT350::Shape>& Player::GetShapes()
{
    return mShapes;
}
