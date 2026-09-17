#include "Bullet.h"

Bullet::Bullet(CMPUT350::Point2D location, CMPUT350::Point2D heading, bool player)
{
}

bool Bullet::IsPlayerBullet()
{
    // TODO: Update
    return true;
}

void Bullet::Initialize(CMPUT350::GameContext* context)
{
}

void Bullet::Update(CMPUT350::GameContext* context)
{
}

void Bullet::LateUpdate(CMPUT350::GameContext* context)
{
}

bool Bullet::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
}

void Bullet::RenderBackground(CMPUT350::GameContext* context)
{
}

void Bullet::RenderForeground(CMPUT350::GameContext* context)
{
}

void Bullet::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
}

void Bullet::Kill()
{
}

bool Bullet::IsAlive() const
{
    // TODO: Update code
    return true;
}

const CMPUT350::Rect& Bullet::GetBounds()
{
    // TODO: Update code
    static CMPUT350::Rect sBounds(0, 0, 0, 0);
    return sBounds;
}
