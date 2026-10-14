#include "Enemy.h"
#include "Bullet.h"

//-----------------------------------------------------------------------
/**
 * @brief Creates an enemy with a fixed 30x30 hitbox centred on its location.
 * @param loc Centre of the enemy in world coordinates.
 */
//-----------------------------------------------------------------------
Enemy::Enemy(CMPUT350::Point2D loc):mLoc(loc), mBounds(loc - 15.0f, 30.0f, 30.0f), mAlive(true)
{
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Circle(0.0f, 0.0f, 15.0f)));
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
    return false;
}

void Enemy::RenderBackground(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the enemy as a filled red square.
 *
 * The square is centred on the enemy's local origin; the engine moves the
 * DrawContext to the enemy's world position before this runs. Its collision
 * shape is a circle, so the drawn square and the collision outline are only
 * approximate matches.
 * @param context Supplies the drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Enemy::RenderForeground(CMPUT350::GameContext* context)
{
    context->ScreenContext->DrawRect(CMPUT350::Rect(-15.0f, -15.0f, 30.0f, 30.0f), CMPUT350::Colors::red);
}

CMPUT350::Point2D Enemy::GetLocation() const
{
    return mLoc;
}

float Enemy::GetRotation() const
{
    return 0.0;
}

//-----------------------------------------------------------------------
/**
 * @brief Destroys the enemy when struck by a player bullet.
 *
 * The bullet's own side is checked so enemy fire cannot remove an enemy.
 * @param obj The object whose bounds overlapped this enemy.
 * @return None. Kills the enemy on a hit.
 */
//-----------------------------------------------------------------------
void Enemy::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    // nullptr from the cast means the overlap was not a bullet.
    std::shared_ptr<Bullet> bullet = std::dynamic_pointer_cast<Bullet>(obj);

    if (bullet != nullptr && bullet->IsPlayerBullet())
    {
        Kill();
    }
}

void Enemy::Kill()
{
    mAlive = false;
}

bool Enemy::IsAlive() const
{
    return mAlive;
}

const CMPUT350::Rect& Enemy::GetBounds()
{
    return mBounds;
}

const std::vector<CMPUT350::Shape>& Enemy::GetShapes()
{
    return mShapes;
}
