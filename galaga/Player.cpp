#include <cassert>
#include <algorithm>

#include "Player.h"
#include "Bullet.h"
#include "Keyboard.h"
#include "NotificationManager.h"
namespace {

constexpr char kMoveLeftDown[] = "player.move.left.down";
constexpr char kMoveLeftUp[] = "player.move.left.up";

constexpr char kMoveRightDown[] = "player.move.right.down";
constexpr char kMoveRightUp[] = "player.move.right.up";

constexpr char kShoot[] = "player.shoot";

}

//-----------------------------------------------------------------------
/**
 * @brief Creates the player ship with a 40x40 hitbox centred on its location.
 * @param loc Centre of the ship in world coordinates.
 */
//-----------------------------------------------------------------------
Player::Player(CMPUT350::Point2D loc):mLoc(loc), mBounds(loc - 20.0f, 40.0f, 40.0f), mAlive(true), mSpeed(7.0f),
    mMoveLeft(false), mMoveRight(false), mFireRequested(false)
{
    // These hitboxes are local to mLoc; the engine adds GetLocation() when it
    // tests them, so the world position must not be baked in here.
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(-20.0f, 0.0f, 40.0f, 20.0f)));
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(-5.0f, -20.0f, 10.0f, 20.0f)));
}

void Player::Initialize(CMPUT350::GameContext* context)
{
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
 * @brief Moves the ship horizontally by one speed step per held direction.
 *
 * The direction is read from flags set by notifications and applied here, so
 * movement is frame-rate independent: the ship advances a fixed distance each
 * frame rather than once per key event. Holding both keys cancels out.
 *
 * The position is clamped so the 40px hitbox stays inside the 768px window,
 * and topLeft is re-derived afterwards to keep the centre and bounds in sync.
 * @param context Unused by this implementation.
 * @return None. Updates mLoc and mBounds.
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
        // 50/718 keep the 40px hitbox inside the 768px window. Moving the
        // centre and then re-deriving topLeft keeps the two in sync.
        mLoc.x = std::clamp(mLoc.x + direction * mSpeed, 50.0f, 718.0f);
        mBounds.topLeft.x = mLoc.x - 20.0f;
    }
    if (mFireRequested) {
        mFireRequested = false;

        if (b1.expired()) {
            auto bullet = std::make_shared<Bullet>(
                CMPUT350::Point2D(mLoc),
                CMPUT350::Point2D(0.0f, -40.0f),
                true
            );
            b1 = bullet;
            context->EngineContext->AddGameObject(bullet);
        } else if (b2.expired()) {
            auto bullet = std::make_shared<Bullet>(
                CMPUT350::Point2D(mLoc),
                CMPUT350::Point2D(0.0f, -40.0f),
                true
            );
            b2 = bullet;
            context->EngineContext->AddGameObject(bullet);
        }
    }
}

void Player::LateUpdate(CMPUT350::GameContext* context)
{
}

void Player::RenderBackground(CMPUT350::GameContext* context)
{
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

void Player::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
}

void Player::Kill()
{
    mAlive = false;
}

bool Player::IsAlive() const
{
    return mAlive;
}
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

const CMPUT350::Rect& Player::GetBounds()
{
    return mBounds;
}
const std::vector<CMPUT350::Shape>& Player::GetShapes()
{
    return mShapes;
}
