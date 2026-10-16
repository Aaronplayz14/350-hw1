#include "Bullet.h"
#include "Enemy.h"

//-----------------------------------------------------------------------
/**
 * @brief Creates a bullet travelling in a fixed direction each frame.
 *
 * The bullet keeps its previous location so its hitbox can cover the whole
 * segment travelled rather than a single instantaneous position, which is
 * what stops fast bullets from passing through enemies between frames.
 *
 * @param location Starting position of the bullet.
 * @param heading Per-frame displacement added to the position in Update.
 * @param player true for shots fired by the player, false for enemy shots.
 */
//-----------------------------------------------------------------------
Bullet::Bullet(CMPUT350::Point2D location, CMPUT350::Point2D heading, bool player): 
    mLocation(location), mPreviousLocation(location), mHeading(heading), mPlayer(player), mAlive(true), mBounds(location, location)
{
    // Segment points from where the bullet was toward where it is now, so the
    // collision line matches the direction the bullet just travelled.
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Line(-(heading.x), -(heading.y), 0.0f, 0.0f)));
}

//-----------------------------------------------------------------------
/**
 * @brief Reports which side fired this bullet.
 *
 * Enemies ignore enemy shots so they do not destroy each other, and player
 * bullets only react to enemies.
 * @return true if fired by the player, false if fired by an enemy.
 */
//-----------------------------------------------------------------------
bool Bullet::IsPlayerBullet() const
{
    // TODO: Update
    return mPlayer;
}

void Bullet::Initialize(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Advances the bullet one frame and retires it off the screen edge.
 *
 * The previous location is saved before the move and the hitbox is rebuilt as
 * the rectangle spanning the old and new positions. A bullet covering only
 * its current position would skip past a small enemy entirely, because the
 * engine tests collisions after movement.
 *
 * @param context Supplies the window size used for the bounds check.
 * @return None. Marks the bullet dead once it leaves the window.
 */
//-----------------------------------------------------------------------
void Bullet::Update(CMPUT350::GameContext* context)
{
    // Save the old spot before moving: the hitbox below spans both positions.
    mPreviousLocation = mLocation;
    mLocation += mHeading;

    
    const float windowWidth = static_cast<float>(context->ScreenContext->GetWindowWidth());

    const float windowHeight = static_cast<float>(context->ScreenContext->GetWindowHeight());

    if (mLocation.x < 0.0f || mLocation.x >= windowWidth ||
        mLocation.y < 0.0f || mLocation.y >= windowHeight)
    {
        mAlive = false;
    }

    // Swept bounds: covers the whole segment travelled this frame, so a fast
    // bullet cannot skip past a small target between collision checks.
    mBounds = CMPUT350::Rect(mPreviousLocation, mLocation);
}

void Bullet::LateUpdate(CMPUT350::GameContext* context)
{
}

void Bullet::RenderBackground(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the bullet as a short streak along the segment it just moved.
 *
 * Drawing the segment travelled rather than a dot keeps fast bullets visible.
 * Both endpoints are expressed relative to the bullet's current location,
 * because the engine applies that location as the drawing offset.
 * @param context Supplies the drawing context.
 * @return None. Colour is yellow for player shots, cyan for enemy shots.
 */
//-----------------------------------------------------------------------
void Bullet::RenderForeground(CMPUT350::GameContext* context)
{
    const CMPUT350::RGBColor color = mPlayer ? CMPUT350::Colors::yellow : CMPUT350::Colors::cyan;

    // The current position is the local origin, so shift the previous world
    // position by it to get the streak's other end.
    const CMPUT350::Point2D previousLocal = mPreviousLocation - mLocation;

    context->ScreenContext->DrawLine(previousLocal, CMPUT350::Point2D(0.0f, 0.0f), 3.0f, color);
}

CMPUT350::Point2D Bullet::GetLocation() const
{
    return mLocation;
}

float Bullet::GetRotation() const
{
    return 0.0;
}

//-----------------------------------------------------------------------
/**
 * @brief Consumes the bullet when it hits an enemy.
 *
 * Only player bullets are absorbed. Enemy shots pass through enemies so that
 * a formation firing at once cannot destroy itself.
 * @param obj The object whose bounds overlapped this bullet.
 * @return None. Kills the bullet on a player-shot/enemy hit.
 */
//-----------------------------------------------------------------------
void Bullet::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    // Cast first: the cast is also the type test, so a nullptr means "not an
    // enemy" without a second check.
    if (!mPlayer)
    {
        return;
    }

    auto enemy = std::dynamic_pointer_cast<Enemy>(obj);

    if (enemy != nullptr && enemy->IsAlive())
    {
        Kill();
    }
}

void Bullet::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj, const CMPUT350::Point2D& collisionPoint)
{
    CollisionEnter(obj);
}

void Bullet::Kill()
{
    mAlive = false;
}

bool Bullet::IsAlive() const
{
    return mAlive;
}

const CMPUT350::Rect& Bullet::GetBounds()
{
    return mBounds;
}

const std::vector<CMPUT350::Shape>& Bullet::GetShapes()
{
    return mShapes;
}
