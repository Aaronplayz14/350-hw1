#include <cassert>
#include <algorithm>

#include "Player.h"
#include "Bullet.h"

Player::Player(CMPUT350::Point2D loc):mLoc(loc), mBounds(loc - 20.0f, 40.0f, 40.0f), mAlive(true), mSpeed(7.0f),
    mMoveLeft(false), mMoveRight(false)
{
    // TODO: Update code
}

void Player::Initialize(CMPUT350::GameContext* context)
{
}

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

void Player::HandleKeyState(CMPUT350::GameContext* context, char key, bool pressed)
{
    if (key == 'a') {
        mMoveLeft = pressed;
    } else if (key == 'd') {
        mMoveRight = pressed;
    }
}

void Player::LateUpdate(CMPUT350::GameContext* context)
{
}

bool Player::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
    if(key == ' '){
        if(b1.expired()){
            auto sharedBullet = std::make_shared<Bullet>(CMPUT350::Point2D(mLoc), CMPUT350::Point2D(0.0f, -20.0f), true);
            b1 = sharedBullet;
            context->EngineContext->AddGameObject(sharedBullet);
        } else if (b2.expired()){
            auto sharedBullet = std::make_shared<Bullet>(CMPUT350::Point2D(mLoc), CMPUT350::Point2D(0.0f, -20.0f), true);
            b2 = sharedBullet;
            context->EngineContext->AddGameObject(sharedBullet);
        }
    }

    return false;
}

void Player::RenderBackground(CMPUT350::GameContext* context)
{
}

void Player::RenderForeground(CMPUT350::GameContext* context)
{
    context->ScreenContext->DrawRect(mBounds, CMPUT350::Colors::blue);
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
