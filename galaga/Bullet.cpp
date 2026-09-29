#include "Bullet.h"
#include "Enemy.h"

Bullet::Bullet(CMPUT350::Point2D location, CMPUT350::Point2D heading, bool player): 
    mLocation(location), mPreviousLocation(location), mHeading(heading), mPlayer(player), mAlive(true), mBounds(location, location)
{
}

bool Bullet::IsPlayerBullet()
{
    // TODO: Update
    return mPlayer;
}

void Bullet::Initialize(CMPUT350::GameContext* context)
{
}

void Bullet::Update(CMPUT350::GameContext* context)
{
    mPreviousLocation = mLocation;
    mLocation += mHeading;

    CMPUT350::Point2D pos = mLocation;

    if (pos.x < 0 || pos.x > static_cast<float>(context->ScreenContext->GetWindowWidth()) ||
        pos.y < 0 || pos.y > static_cast<float>(context->ScreenContext->GetWindowHeight()))
    {
        mAlive = false;
    }

    mBounds = CMPUT350::Rect(mPreviousLocation, mLocation);
}

void Bullet::LateUpdate(CMPUT350::GameContext* context)
{
}

bool Bullet::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
    return false;
}

void Bullet::RenderBackground(CMPUT350::GameContext* context)
{
}

void Bullet::RenderForeground(CMPUT350::GameContext* context)
{
    CMPUT350::RGBColor color = mPlayer ? CMPUT350::Colors::yellow : CMPUT350::Colors::cyan;

    context->ScreenContext->DrawLine( mPreviousLocation, mLocation, 3.0f, color);
}

void Bullet::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    std::shared_ptr<Enemy> enemy = std::dynamic_pointer_cast<Enemy>(obj);

    if (enemy != nullptr && mPlayer)
    {
        Kill();
    }
}

void Bullet::Kill()
{
    mAlive = false;
}

bool Bullet::IsAlive() const
{
    // TODO: Update code
    return mAlive;
}

const CMPUT350::Rect& Bullet::GetBounds()
{
    // TODO: Update code
    return mBounds;
}
