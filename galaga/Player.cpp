#include <cassert>
#include <algorithm>

#include "Player.h"
#include "Bullet.h"

//-----------------------------------------------------------------------
/**
 * @brief Creates the player ship with a 40x40 hitbox centred on its location.
 * @param loc Centre of the ship in world coordinates.
 */
//-----------------------------------------------------------------------
Player::Player(CMPUT350::Point2D loc):mLoc(loc), mBounds(loc - 20.0f, 40.0f, 40.0f), mAlive(true), mSpeed(7.0f),
    mMoveLeft(false), mMoveRight(false)
{
    // TODO: Update code
}

void Player::Initialize(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Moves the ship horizontally by one speed step per held direction.
 *
 * The direction is read from flags set by HandleKeyState and applied here, so
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
}

//-----------------------------------------------------------------------
/**
 * @brief Records which horizontal direction is currently held.
 *
 * Only stores state; the actual movement happens in Update. Without this the
 * ship would jump a single step per key event instead of travelling while
 * the key is down.
 *
 * @param context Unused by this implementation.
 * @param key 'a' for left or 'd' for right; any other key is ignored.
 * @param pressed true on key down, false on key release.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::HandleKeyState(CMPUT350::GameContext* context, char key, bool pressed)
{
    // Only record state; Update applies it, so holding a key
    // moves continuously instead of one step per key event.
    if (key == 'a') {
        mMoveLeft = pressed;
    } else if (key == 'd') {
        mMoveRight = pressed;
    }
}

void Player::LateUpdate(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Fires an upward shot when space is pressed.
 *
 * Two bullet slots are held as weak pointers. The engine owns the bullets, so
 * a slot expiring is how the player learns it may fire again; holding the
 * slots weakly avoids a cycle that would keep dead bullets alive. Presses
 * beyond the two available slots are ignored rather than queued.
 *
 * @param context Supplies the engine used to add the new bullet.
 * @param key The character pressed; only a space triggers a shot.
 * @return false, so the key is left unconsumed.
 */
//-----------------------------------------------------------------------
bool Player::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
    if(key == ' '){
        // expired() means the engine has already dropped that bullet, so the
        // slot is reusable. Both slots busy means the press is ignored.
        if(b1.expired()){
            auto sharedBullet = std::make_shared<Bullet>(CMPUT350::Point2D(mLoc), CMPUT350::Point2D(0.0f, -40.0f), true);
            b1 = sharedBullet;
            context->EngineContext->AddGameObject(sharedBullet);
        } else if (b2.expired()){
            auto sharedBullet = std::make_shared<Bullet>(CMPUT350::Point2D(mLoc), CMPUT350::Point2D(0.0f, -40.0f), true);
            b2 = sharedBullet;
            context->EngineContext->AddGameObject(sharedBullet);
        }
    }

    return false;
}

void Player::RenderBackground(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the ship as a blocky Galaga-style sprite.
 *
 * Ten rectangles are placed relative to mLoc: white blocks form the hull and
 * wings, red blocks mark the cockpit and engine, and a blue block forms the
 * base. The hitbox stays a single 40x40 rectangle rather than matching these
 * blocks exactly, which is the usual simplification for ship collision.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Player::RenderForeground(CMPUT350::GameContext* context)
{
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-5.0f, -15.0f), 10.0f, 20.0f), CMPUT350::Colors::white);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-10.0f, -5.0f), 5.0f, 15.0f), CMPUT350::Colors::white);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(5.0f, -5.0f), 5.0f, 15.0f), CMPUT350::Colors::white);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-15.0f, 10.0f), 30.0f, 10.0f), CMPUT350::Colors::white);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-20.0f, 5.0f), 5.0f, 10.0f), CMPUT350::Colors::white);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(15.0f, 5.0f), 5.0f, 10.0f), CMPUT350::Colors::white);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-20.0f, 0.0f), 5.0f, 5.0f), CMPUT350::Colors::red);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(15.0f, 0.0f), 5.0f, 5.0f), CMPUT350::Colors::red);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-5.0f, -20.0f), 10.0f, 5.0f), CMPUT350::Colors::red);
    context->ScreenContext->DrawRect(CMPUT350::Rect(mLoc + CMPUT350::Point2D(-5.0f, 5.0f), 10.0f, 5.0f), CMPUT350::Colors::blue);
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
    // TODO: Update code
    return mAlive;
}

const CMPUT350::Rect& Player::GetBounds()
{
    // TODO: Update code
    return mBounds;
}
