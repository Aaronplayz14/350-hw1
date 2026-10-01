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
bool Bullet::IsPlayerBullet()
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

    // Copied so the offscreen test reads the new position without mutating
    // mLocation itself.
    CMPUT350::Point2D pos = mLocation;

    if (pos.x < 0 || pos.x > static_cast<float>(context->ScreenContext->GetWindowWidth()) ||
        pos.y < 0 || pos.y > static_cast<float>(context->ScreenContext->GetWindowHeight()))
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

bool Bullet::HandleKeyEvent(CMPUT350::GameContext* context, char key)
{
    return false;
}

void Bullet::RenderBackground(CMPUT350::GameContext* context)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the bullet as a short streak along the segment it just moved.
 *
 * Drawing the segment travelled rather than a dot keeps fast bullets visible.
 * @param context Supplies the drawing context.
 * @return None. Colour is yellow for player shots, cyan for enemy shots.
 */
//-----------------------------------------------------------------------
void Bullet::RenderForeground(CMPUT350::GameContext* context)
{
    CMPUT350::RGBColor color = mPlayer ? CMPUT350::Colors::yellow : CMPUT350::Colors::cyan;

    context->ScreenContext->DrawLine( mPreviousLocation, mLocation, 3.0f, color);
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
    std::shared_ptr<Enemy> enemy = std::dynamic_pointer_cast<Enemy>(obj);

    if (enemy != nullptr && mPlayer)
    {
        this->Kill();
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
