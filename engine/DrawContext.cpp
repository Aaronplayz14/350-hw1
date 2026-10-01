#include "DrawContext.h"

#include <cmath>

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Creates a drawing context bound to an existing window and font.
 *
 * The context does not own the window or font; both are held by the
 * GameEngine that constructs this object and are expected to outlive it.
 *
 * @param window The already-open SFML render window to draw onto.
 * @param font The font used by DrawText and DrawCenteredText.
 */
//-----------------------------------------------------------------------
DrawContext::DrawContext(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font)
    : mWindow(window), mFont(font) {}

//-----------------------------------------------------------------------
/**
 * @brief Draws a line of text with its top-left corner at a given point.
 *
 * The pixel size is applied to the embedded font rather than scaling an
 * existing image, so the glyphs stay crisp at every size.
 *
 * @param text The string to render.
 * @param pixelSize Height of the glyphs in pixels.
 * @param p The top-left position of the text in world coordinates.
 * @param c The fill color of the glyphs.
 * @return None. The text is drawn immediately onto the window.
 *
 * @see DrawCenteredText for the variant that centres on p instead.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawText(const std::string& text, int pixelSize, Point2D p, RGBColor c)
{
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    label.setFillColor(sf::Color(c.r, c.g, c.b));
    label.setPosition({p.x, p.y});

    mWindow->draw(label);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a line of text centred on a given point.
 *
 * SFML positions a shape by its top-left corner, so the local bounds are
 * measured and the origin is shifted by half their size before the shape is
 * moved to p. Without this the text would sit down and to the right of p.
 *
 * @param text The string to render.
 * @param pixelSize Height of the glyphs in pixels.
 * @param p The point that should sit at the centre of the text.
 * @param c The fill color of the glyphs.
 * @return None. The text is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawCenteredText(const std::string& text, int pixelSize, Point2D p, RGBColor c)
{
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    label.setFillColor(sf::Color(c.r, c.g, c.b));

    // Shift the text so its center sits exactly on p.
    sf::FloatRect bounds = label.getLocalBounds();
    label.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                     bounds.position.y + bounds.size.y / 2.0f});
    label.setPosition({p.x, p.y});

    mWindow->draw(label);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a filled circle.
 *
 * sf::CircleShape is anchored at its own top-left corner, so the origin is
 * moved to the radius to make p the centre of the circle rather than the
 * corner of its bounding box.
 *
 * @param p The centre of the circle in world coordinates.
 * @param radius The radius of the circle in pixels.
 * @param c The fill color of the circle.
 * @return None. The circle is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawCircle(Point2D p, float radius, RGBColor c)
{
    sf::CircleShape circle(radius);
    circle.setFillColor(sf::Color(c.r, c.g, c.b));
    circle.setOrigin({radius, radius});
    circle.setPosition({p.x, p.y});

    mWindow->draw(circle);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a solid rectangle.
 *
 * The Rect stores its top-left corner rather than a centre, which matches the
 * way SFML positions a RectangleShape, so no origin adjustment is needed.
 *
 * @param r The rectangle to fill, in world coordinates.
 * @param c The fill color of the rectangle.
 * @return None. The rectangle is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawRect(Rect r, RGBColor c)
{
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color(c.r, c.g, c.b));
    rect.setPosition({r.topLeft.x, r.topLeft.y});

    mWindow->draw(rect);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the outline of a rectangle with its interior left empty.
 *
 * The fill is set transparent and the colour is applied to the outline
 * instead, which keeps the drawn pixels on the border of r only.
 *
 * @param r The rectangle to outline, in world coordinates.
 * @param width The thickness of the border in pixels, drawn centred on the edge.
 * @param c The color of the border.
 * @return None. The outline is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::FrameRect(Rect r, float width, RGBColor c)
{
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(width);
    rect.setOutlineColor(sf::Color(c.r, c.g, c.b));
    rect.setPosition({r.topLeft.x, r.topLeft.y});

    mWindow->draw(rect);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a line segment of a given width between two points.
 *
 * SFML has no line primitive, so the segment is built as a four-point convex
 * quad. The two corners at each end are offset by half the width along the
 * normal to the segment, which keeps the drawn quad spanning exactly from
 * "from" to "to" instead of trailing behind either endpoint.
 *
 * A zero-length segment has no direction to offset along, so the normal falls
 * back to zero and the result collapses to a single point.
 *
 * @param from The starting point of the segment (Point2D).
 * @param to The ending point of the segment (Point2D).
 * @param width The thickness of the line in pixels.
 * @param c The color of the line (RGBColor).
 * @return None. The line is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawLine(Point2D from, Point2D to, float width, RGBColor c)
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float length = std::sqrt(dx * dx + dy * dy);

    // SFML has no line primitive, so build a thin quad along the segment.
    // Half width perpendicular to the direction is the only offset needed.
    float nx = 0.0f;
    float ny = 0.0f;
    if (length > 0.0f) {
        nx = -dy / length * width / 2.0f;
        ny = dx / length * width / 2.0f;
    }

    sf::ConvexShape line;
    line.setPointCount(4);
    line.setPoint(0, sf::Vector2f(from.x + nx, from.y + ny));
    line.setPoint(1, sf::Vector2f(to.x + nx, to.y + ny));
    line.setPoint(2, sf::Vector2f(to.x - nx, to.y - ny));
    line.setPoint(3, sf::Vector2f(from.x - nx, from.y - ny));
    line.setFillColor(sf::Color(c.r, c.g, c.b));

    mWindow->draw(line);
}

//-----------------------------------------------------------------------
/**
 * @brief Reports the width of the window in pixels.
 *
 * @return The horizontal size of the render window as configured at startup.
 */
//-----------------------------------------------------------------------
int DrawContext::GetWindowWidth() { return mWindow->getSize().x; }

//-----------------------------------------------------------------------
/**
 * @brief Reports the height of the window in pixels.
 *
 * @return The vertical size of the render window as configured at startup.
 */
//-----------------------------------------------------------------------
int DrawContext::GetWindowHeight() { return mWindow->getSize().y; }

}  // namespace CMPUT350