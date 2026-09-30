#include <cassert>
#include "Player.h"
#include "Bullet.h"

Player::Player(CMPUT350::Point2D loc):mLoc(loc), mBounds(loc - 20.0f, 40.0f, 40.0f), mAlive(true), mSpeed(5.0f)
{
    // TODO: Update code
}

void Player::Initialize(CMPUT350::GameContext* context)
{
}

void Player::Update(CMPUT350::GameContext* context)
{
}

void Player::LateUpdate(CMPUT350::GameContext* context)
{
}

bool Player::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
    if(key == 'a' || key == 'A'){
        if(mLoc.x - mSpeed >= 50 && mLoc.x - mSpeed <= 718){
            mLoc.x -= mSpeed;
            mBounds.topLeft.x -= mSpeed;
        }
    }

    if(key == 'd' || key == 'D'){
        if(mLoc.x + mSpeed >= 50 && mLoc.x + mSpeed <= 718){
            mLoc.x += mSpeed;
            mBounds.topLeft.x += mSpeed;
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
