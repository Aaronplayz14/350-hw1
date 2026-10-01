#include "GameContext.h"
#include "DrawContext.h"
#include "Stars.h"

//-----------------------------------------------------------------------
/**
 * @brief Pre-generates the background starfield at fixed random positions.
 *
 * Positions are chosen once in the constructor rather than per frame, so the
 * same stars persist for the whole run instead of flickering.
 *
 * @param numStars How many stars to place inside the bounds.
 * @param bounds Region the stars are scattered across and scroll within.
 */
//-----------------------------------------------------------------------
Stars::Stars(int numStars, CMPUT350::Rect bounds)
    : mBounds(bounds), gen(rd())
{
    // Distributions are built once and reused per star, so each draw samples
    // independently but no new distribution is constructed per iteration.
    std::uniform_int_distribution<int> mXRand(bounds.topLeft.x, bounds.topLeft.x + bounds.width);
    std::uniform_int_distribution<int> mYRand(bounds.topLeft.y, bounds.topLeft.y + bounds.height);
    // Generate random star positions within the bounds. Done once here rather
    // than per frame so the field is stable instead of flickering.
    for (int x = 0; x < numStars; x++)
    {
        mStarPositions.emplace_back(mXRand(gen), mYRand(gen));
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the starfield and scrolls it downward each frame.
 *
 * Each star moves down a few pixels and wraps back to the top once it passes
 * the bottom edge, producing continuous motion with no allocation. A counter
 * advances every five frames and skips one star in each group, which reads as
 * occasional twinkling as stars are dropped from the cycle.
 *
 * Rendered as a background pass, so the field sits behind ships and bullets.
 * @param context Supplies the drawing context.
 * @return None. Star positions are advanced as a side effect.
 */
//-----------------------------------------------------------------------
void Stars::RenderBackground(CMPUT350::GameContext* context)
{
    CMPUT350::RGBColor c[4] = {
        CMPUT350::Colors::white, CMPUT350::Colors::cyan, CMPUT350::Colors::magenta, CMPUT350::Colors::white
    };
    // skip counts frames and only advances every 5, so the pattern below
    // changes slowly and reads as twinkling rather than flicker.
    static int skip = 0;
    int curr = 0;
    skip++;
    for (auto& star : mStarPositions)
    {
        curr++;
        // Skipping one star per group makes stars blink out briefly.
        if ((curr + skip / 5) % (mStarPositions.size() / 20) != 0)
            context->ScreenContext->DrawCircle(star, 1.0f, c[(curr) % 4]);
        // Wrap by the full height once past the bottom, reusing the same rows.
        star.y += 3;
        if (star.y > mBounds.topLeft.y + mBounds.height)
            star.y -= mBounds.height;
    }
}
