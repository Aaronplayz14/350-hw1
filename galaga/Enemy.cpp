#include "Enemy.h"
#include "Bullet.h"

Enemy::Enemy(CMPUT350::Point2D loc)
{
    // TODO: Update code
}

void Enemy::Initialize(CMPUT350::GameContext* context)
{
}

void Enemy::Update(CMPUT350::GameContext* context)
{
}

void Enemy::LateUpdate(CMPUT350::GameContext* context)
{
}

bool Enemy::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
}

void Enemy::RenderBackground(CMPUT350::GameContext* context)
{
}

void Enemy::RenderForeground(CMPUT350::GameContext* context)
{
}

void Enemy::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
}

void Enemy::Kill()
{
}

bool Enemy::IsAlive() const
{
    // TODO: Update code
    return true;
}

const CMPUT350::Rect& Enemy::GetBounds()
{
    // TODO: Update code
    static CMPUT350::Rect sBounds(0, 0, 0, 0);
    return sBounds;
}
