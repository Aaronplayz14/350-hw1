#include "Bullet.h"
#include "Enemy.h"
#include "Player.h"
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
Bullet::Bullet(CMPUT350::Point2D location, CMPUT350::Vector2D heading, bool player,
    std::weak_ptr<CMPUT350::GameObject> parent):
    mLocation(location), mPreviousLocation(location), mHeading(heading), mPlayer(player), mParent(parent), mAlive(true), mDestroyed(false), mHasCollisionPoint(false), mCollisionPoint(0.0f, 0.0f), mBounds(location, location)
{
    // Segment points from where the bullet was toward where it is now, so the
    // collision line matches the direction the bullet just travelled.
    mShapes.push_back(CMPUT350::Shape(CMPUT350::Line(0.0f, 0.0f, -(heading.x), -(heading.y))));
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
    return mPlayer;
}

//-----------------------------------------------------------------------
/**
 * @brief No setup is required; retained to satisfy the GameObject interface.
 * @param context Unused.
 * @return None.
 */
//-----------------------------------------------------------------------
void Bullet::Initialize(CMPUT350::GameContext* context)
{
    (void)context;
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
    if (!mAlive)
    {
        return;
    }
    const auto parent = mParent.lock();

    if (parent == nullptr || !parent->IsAlive())
    {
        mAlive = false;
        return;
    }
    // Save the old spot before moving: the hitbox below spans both positions.
    mPreviousLocation = mLocation;
    mLocation += mHeading;

    // Swept bounds: covers the whole segment travelled this frame, so a fast
    // bullet cannot skip past a small target between collision checks. Set
    // before the early-out below so a valid box exists even without a context.
    mBounds = CMPUT350::Rect(mPreviousLocation, mLocation);
    if (context == nullptr || context->ScreenContext == nullptr)
    {
        return;
    }
    const float windowWidth = static_cast<float>(context->ScreenContext->GetWindowWidth());

    const float windowHeight = static_cast<float>(context->ScreenContext->GetWindowHeight());

    if (mLocation.x < 0.0f || mLocation.x >= windowWidth ||
        mLocation.y < 0.0f || mLocation.y >= windowHeight)
    {
        mAlive = false;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Turns a bullet that hit something into a one-frame impact marker.
 *
 * Collisions are resolved after Update, so the bullet cannot be removed there;
 * flagging it here lets RenderForeground draw the impact dot for this frame
 * and the engine remove the bullet at the start of the next.
 * @param context Unused.
 * @return None.
 */
//-----------------------------------------------------------------------
void Bullet::LateUpdate(CMPUT350::GameContext* context)
{
    (void)context;

    if (mDestroyed)
    {
        mAlive = false;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Bullets are always drawn on the foreground layer.
 * @param context Unused.
 * @return None.
 */
//-----------------------------------------------------------------------
void Bullet::RenderBackground(CMPUT350::GameContext* context)
{
    (void)context;
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the bullet as a short streak along the segment it just moved.
 *
 * Drawing the segment travelled rather than a dot keeps fast bullets visible.
 * Both endpoints are expressed relative to the bullet's current location,
 * because the engine applies that location as the drawing offset.
 * @param context Supplies the drawing context.
 * @return None. Colour is yellow for player shots, red for enemy shots.
 */
//-----------------------------------------------------------------------
void Bullet::RenderForeground(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->ScreenContext == nullptr)
    {
        return;
    }
    if (!mAlive && !mDestroyed)
    {
        return;
    }

    const CMPUT350::RGBColor color = mPlayer ? CMPUT350::Colors::yellow : CMPUT350::Colors::red;
    if (mDestroyed && mHasCollisionPoint)
    {
        // CollisionPoint is local to this bullet.
        context->ScreenContext->DrawCircle(
            mCollisionPoint,
            8.0f,
            color
        );
    }
    else if (!mDestroyed)
    {
        const CMPUT350::Point2D previousLocal = mPreviousLocation - mLocation;
        if (mPlayer)
        {
            context->ScreenContext->DrawLine(previousLocal, CMPUT350::Point2D(0.0f, 0.0f), 6.0f, color);
        }
        else
        {
            const CMPUT350::Point2D streakEnd = previousLocal * 4.0f;
            context->ScreenContext->DrawLine(CMPUT350::Point2D(0.0f, 0.0f), streakEnd, 5.0f, color);
        }
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Reports the bullet's current world position for drawing.
 * @return The centre of the bullet.
 */
//-----------------------------------------------------------------------
CMPUT350::Point2D Bullet::GetLocation() const
{
    return mLocation;
}

//-----------------------------------------------------------------------
/**
 * @brief Bullets never rotate, so their drawing rotation is always zero.
 * @return 0 radians.
 */
//-----------------------------------------------------------------------
float Bullet::GetRotation() const
{
    return 0.0;
}

//-----------------------------------------------------------------------
/**
 * @brief Handles a collision that has no single well-defined contact point.
 *
 * This is the fallback used for shape pairs such as circle/rectangle that meet
 * without a crossing point. The impact is recorded at the bullet's own origin
 * so the destroyed marker still has somewhere to draw.
 * @param obj The object whose bounds overlapped this bullet.
 * @return None.
 */
//-----------------------------------------------------------------------
void Bullet::CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj)
{
    if (obj == nullptr || !mAlive)
    {
        return;
    }

    // Bullets must not collide with the object that created them.
    const auto parent = mParent.lock();

    if (parent != nullptr &&
        parent.get() == static_cast<CMPUT350::GameObject*>(obj.get()))
    {
        return;
    }

    const auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
    const auto player = std::dynamic_pointer_cast<Player>(obj);

    // Player fire hits enemies, and also a friendly ship that has been
    // captured (shooting your own stolen fighter destroys it). Enemy fire only
    // hits the player.
    const bool validTarget =
        (mPlayer && enemy != nullptr) ||
        (!mPlayer && player != nullptr) ||
        (mPlayer && player != nullptr && player->IsCaptured());

    if (!validTarget)
    {
        return;
    }
    if (!mHasCollisionPoint)
    {
        // Shape pairs without a unique contact point fall back to the bullet origin.
        mCollisionPoint = CMPUT350::Point2D(0.0f, 0.0f);
        mHasCollisionPoint = true;
    }

    mDestroyed = true;
}

//-----------------------------------------------------------------------
/**
 * @brief Handles a collision that provides an exact contact point.
 *
 * The engine passes the point in this bullet's local coordinates. When a
 * bullet overlaps more than one shape the requirement is to keep the impact
 * with the largest y value, so an existing point is only replaced by one that
 * sits lower on the screen.
 * @param obj The object whose bounds overlapped this bullet.
 * @param collisionPoint Contact point in local coordinates.
 * @return None.
 */
//-----------------------------------------------------------------------
void Bullet::CollisionEnter(
    const std::shared_ptr<CMPUT350::CollisionObject>& obj,
    const CMPUT350::Point2D& collisionPoint
)
{
    if (obj == nullptr || !mAlive)
    {
        return;
    }

    const auto parent = mParent.lock();

    if (parent != nullptr &&
        parent.get() == static_cast<CMPUT350::GameObject*>(obj.get()))
    {
        return;
    }

    const auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
    const auto player = std::dynamic_pointer_cast<Player>(obj);

    // Player fire hits enemies, and also a friendly ship that has been
    // captured (shooting your own stolen fighter destroys it). Enemy fire only
    // hits the player.
    const bool validTarget =
        (mPlayer && enemy != nullptr) ||
        (!mPlayer && player != nullptr) ||
        (mPlayer && player != nullptr && player->IsCaptured());

    if (!validTarget)
    {
        return;
    }
    if (!mHasCollisionPoint || collisionPoint.y > mCollisionPoint.y)
    {
        mCollisionPoint = collisionPoint;
        mHasCollisionPoint = true;
    }

    mDestroyed = true;
}

//-----------------------------------------------------------------------
/**
 * @brief Removes the bullet immediately.
 * @return None.
 */
//-----------------------------------------------------------------------
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
