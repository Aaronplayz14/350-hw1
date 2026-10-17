#include "Player.h"

#include <algorithm>
#include <cmath>

#include "Bullet.h"
#include "Enemy.h"
#include "NotificationManager.h"

namespace {

// Half the ship's width, used to keep the hull inside the window.
constexpr float kHalfWidth = 20.0f;

}  // namespace

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
      mNotificationManager(nullptr) {
    RefreshBounds();
}

//-----------------------------------------------------------------------
/**
 * @brief Caches the notification manager and subscribes to input events.
 *
 * The engine sets CurrObject to this player before calling Initialize, so the
 * weak self-reference lets the ship register for the movement and shooting
 * notifications installed by the Galaga manager.
 * @param context The engine's per-frame context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Initialize(CMPUT350::GameContext* context) {
    // The engine sets CurrObject to this player before calling Initialize.
    const std::weak_ptr<CMPUT350::GameObject> self = context->CurrObject;

    if (self.expired() || context->mNotificationManager == nullptr) {
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
 * @brief Rebuilds the ship's local hitboxes and world-space bounds.
 *
 * The hull and upper-body rectangles are translated to the ship's current
 * location before calculating the broad-phase bounds.
 * Implemented with the help of AI.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::RefreshBounds() {
    mShapes.clear();
    mShapes.push_back(CMPUT350::Shape(
        CMPUT350::Rect(-kHalfWidth, -kHalfWidth, 2.0f * kHalfWidth, 2.0f * kHalfWidth)));
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Rect(-5.0f, -25.0f, 10.0f, 15.0f)));

    bool firstPoint = true;
    CMPUT350::Point2D minimum;
    CMPUT350::Point2D maximum;
    for (const auto& shape : mShapes) {
        for (const auto& localPoint : shape.shape.rect.Points()) {
            const CMPUT350::Point2D worldPoint = localPoint + mLoc;
            if (firstPoint) {
                minimum = worldPoint;
                maximum = worldPoint;
                firstPoint = false;
            } else {
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
 * @brief Moves the ship in response to the held movement and fire inputs.
 *
 * Direction flags are set by notifications and applied here, so the ship
 * advances a fixed distance per frame rather than once per key event. Holding
 * both keys cancels out. The position is clamped so the hull stays fully inside
 * the playfield, and the bounding box is re-derived afterwards.
 *
 * @param context Supplies the window width and the engine used to spawn shots.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Update(CMPUT350::GameContext* context) {
    if (!mAlive) {
        return;
    }

    float direction = 0.0f;
    if (mMoveLeft) {
        direction -= 1.0f;
    }
    if (mMoveRight) {
        direction += 1.0f;
    }

    if (direction != 0.0f) {
        float minX = kHalfWidth;
        float maxX = 768.0f - kHalfWidth;

        if (context != nullptr && context->ScreenContext != nullptr) {
            maxX = static_cast<float>(context->ScreenContext->GetWindowWidth()) - kHalfWidth;
        }

        mLoc.x = std::clamp(mLoc.x + direction * mSpeed, minX, maxX);
    }

    RefreshBounds();

    if (mFireRequested) {
        mFireRequested = false;
        Fire(context);
    }
}

void Player::LateUpdate(CMPUT350::GameContext* context) { (void)context; }

void Player::RenderBackground(CMPUT350::GameContext* context) { (void)context; }

//-----------------------------------------------------------------------
/**
 * @brief Draws the ship as a blocky Galaga-style sprite.
 *
 * The rectangles are relative to the ship's own origin, since the engine
 * applies mLoc through the DrawContext before this runs.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::RenderForeground(CMPUT350::GameContext* context) {
    if (context == nullptr || context->ScreenContext == nullptr || !mAlive) {
        return;
    }

    auto* screen = context->ScreenContext;

    // Body of one fighter, centred on the given local origin.
    const auto drawFighter = [screen](float originX) {
        screen->DrawRect(CMPUT350::Rect(originX - 5.0f, -15.0f, 10.0f, 20.0f),
                         CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 10.0f, -5.0f, 5.0f, 15.0f),
                         CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX + 5.0f, -5.0f, 5.0f, 15.0f),
                         CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 15.0f, 10.0f, 30.0f, 10.0f),
                         CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 20.0f, 5.0f, 5.0f, 10.0f),
                         CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX + 15.0f, 5.0f, 5.0f, 10.0f),
                         CMPUT350::Colors::white);
        screen->DrawRect(CMPUT350::Rect(originX - 20.0f, 0.0f, 5.0f, 5.0f), CMPUT350::Colors::red);
        screen->DrawRect(CMPUT350::Rect(originX + 15.0f, 0.0f, 5.0f, 5.0f), CMPUT350::Colors::red);
        screen->DrawRect(CMPUT350::Rect(originX - 5.0f, -20.0f, 10.0f, 5.0f),
                         CMPUT350::Colors::red);
        screen->DrawRect(CMPUT350::Rect(originX - 5.0f, 5.0f, 10.0f, 5.0f), CMPUT350::Colors::blue);
    };

    drawFighter(0.0f);
}

CMPUT350::Point2D Player::GetLocation() const { return mLoc; }

float Player::GetRotation() const { return 0.0f; }

//-----------------------------------------------------------------------
/**
 * @brief Handles the ship touching an enemy or enemy bullet.
 *
 * The ship is destroyed by enemies and enemy bullets. Its own bullets pass
 * through it.
 * @param obj The object that overlapped the ship.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj) {
    if (!mAlive || obj == nullptr) {
        return;
    }

    const bool hitByEnemy = std::dynamic_pointer_cast<Enemy>(obj) != nullptr;
    const auto bullet = std::dynamic_pointer_cast<Bullet>(obj);
    const bool hitByEnemyBullet = bullet != nullptr && !bullet->IsPlayerBullet();

    if (hitByEnemy || hitByEnemyBullet) {
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
void Player::Kill() {
    if (!mAlive) {
        return;
    }

    mAlive = false;
    if (mNotificationManager != nullptr) {
        mNotificationManager->Notify(kDestroyedNotification);
    }
}

bool Player::IsAlive() const { return mAlive; }

//-----------------------------------------------------------------------
/**
 * @brief Spawns bullets from the ship's cannons.
 *
 * A single fighter may have two shots in flight. Expired slots are discarded
 * first, and no bullet is fired when both slots are full.
 * @param context Supplies the engine used to add the bullets.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::Fire(CMPUT350::GameContext* context) {
    if (context == nullptr || context->mEngineView == nullptr) {
        return;
    }

    // Forget bullets that have already left the scene.
    mBullets.erase(
        std::remove_if(mBullets.begin(), mBullets.end(),
                       [](const std::weak_ptr<Bullet>& bullet) { return bullet.expired(); }),
        mBullets.end());

    constexpr std::size_t maxSlots = 2u;
    if (mBullets.size() < maxSlots) {
        auto bullet =
            std::make_shared<Bullet>(mLoc + CMPUT350::Point2D(0.0f, -18.0f),
                                     CMPUT350::Point2D(0.0f, -40.0f), true, context->CurrObject);

        mBullets.push_back(bullet);
        context->mEngineView->AddGameObject(bullet);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Updates the held-direction flags and the fire request.
 *
 * Movement keys simply set or clear a direction flag, and the shoot key sets a
 * one-shot flag that Update consumes.
 * @param notification The notification name delivered by the manager.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::ReceiveNotification(const std::string& notification) {
    if (notification == kMoveLeftDown) {
        mMoveLeft = true;
    } else if (notification == kMoveLeftUp) {
        mMoveLeft = false;
    } else if (notification == kMoveRightDown) {
        mMoveRight = true;
    } else if (notification == kMoveRightUp) {
        mMoveRight = false;
    } else if (notification == kShoot) {
        mFireRequested = true;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Returns the ship's world-space bounding box for broad-phase checks.
 * @return The current bounding box.
 */
//-----------------------------------------------------------------------
const CMPUT350::Rect& Player::GetBounds() { return mBounds; }

//-----------------------------------------------------------------------
/**
 * @brief Returns the ship's local collision shapes.
 *
 * The shapes are in the ship's local space; the engine offsets them by
 * GetLocation() and applies the (usually zero) rotation when testing.
 * @return The list of rectangles making up the ship's hitbox.
 */
//-----------------------------------------------------------------------
const std::vector<CMPUT350::Shape>& Player::GetShapes() { return mShapes; }
