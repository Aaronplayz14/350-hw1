#include "DrawContext.h"

#include <algorithm>
#include <cmath>

namespace CMPUT350 {

//-----------------------------------------------------------------------
// Construction
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
/**
 * @brief Creates a drawing context for an existing window and font.
 *
 * The context shares ownership of both resources through shared pointers and
 * uses them for drawing text and shapes.
 *
 * @param window The render window to draw onto; must not be null.
 * @param font The font used to draw text; must not be null.
 */
//-----------------------------------------------------------------------
DrawContext::DrawContext(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font)
    : mWindow(window), mFont(font) {}

//-----------------------------------------------------------------------
// Transform state
//-----------------------------------------------------------------------

void DrawContext::SetContextOffset(Point2D offset) { mContextOffset = offset; }

void DrawContext::SetContextRotation(float rotation) { mContextRotation = rotation; }

//-----------------------------------------------------------------------
/**
 * @brief Converts a point from the object's local space into world space.
 *
 * The local point is rotated first and then shifted by the context offset,
 * which matches how a game object thinks about its own geometry.
 *
 * @param p A point in the current object's local coordinates.
 * @return The same point in world coordinates.
 */
//-----------------------------------------------------------------------
Point2D DrawContext::Transform(Point2D p) const {
    const float cosine = std::cos(mContextRotation);
    const float sine = std::sin(mContextRotation);

    return Point2D(mContextOffset.x + p.x * cosine - p.y * sine,
                   mContextOffset.y + p.x * sine + p.y * cosine);
}

//-----------------------------------------------------------------------
/**
 * @brief Converts a world point back into the object's local space.
 *
 * It is the reverse of Transform: the offset is removed before rotating by
 * the negative angle. Handy when world input has to be expressed relative to
 * a rotated object.
 *
 * @param p A point in world coordinates.
 * @return The same point in the current object's local coordinates.
 */
//-----------------------------------------------------------------------
Point2D DrawContext::ReverseTransform(Point2D p) const {
    // Undo the shift first, so what is left is the rotation component.
    const float x = p.x - mContextOffset.x;
    const float y = p.y - mContextOffset.y;

    const float cosine = std::cos(mContextRotation);
    const float sine = std::sin(mContextRotation);

    return Point2D(x * cosine + y * sine, -x * sine + y * cosine);
}

//-----------------------------------------------------------------------
/**
 * @brief Produces the upright world box that encloses a local rectangle.
 *
 * A rotated rectangle no longer lines up with the axes, so each corner is
 * transformed and the results are wrapped in the tightest axis-aligned box.
 * This is useful for broad-phase style checks and geometry that needs a
 * regular Rect rather than a rotated one.
 *
 * @param r A rectangle in the current object's local coordinates.
 * @return An axis-aligned bounding box in world coordinates.
 */
//-----------------------------------------------------------------------
Rect DrawContext::Transform(Rect r) const {
    // Rotating tilts each corner, so transform all four before measuring.
    const Point2D topLeft = Transform(r.topLeft);
    const Point2D topRight = Transform(Point2D(r.topLeft.x + r.width, r.topLeft.y));
    const Point2D bottomLeft = Transform(Point2D(r.topLeft.x, r.topLeft.y + r.height));
    const Point2D bottomRight = Transform(Point2D(r.topLeft.x + r.width, r.topLeft.y + r.height));

    const float left =
        std::min(std::min(topLeft.x, topRight.x), std::min(bottomLeft.x, bottomRight.x));
    const float right =
        std::max(std::max(topLeft.x, topRight.x), std::max(bottomLeft.x, bottomRight.x));
    const float top =
        std::min(std::min(topLeft.y, topRight.y), std::min(bottomLeft.y, bottomRight.y));
    const float bottom =
        std::max(std::max(topLeft.y, topRight.y), std::max(bottomLeft.y, bottomRight.y));

    return Rect(left, top, right - left, bottom - top);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a line of text with its top-left corner at a given point.
 *
 * @param text The string to render.
 * @param pixelSize Height of the glyphs in pixels.
 * @param p The top-left position of the text in local coordinates.
 * @param c The fill color of the glyphs.
 * @return None. The text is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawText(const std::string& text, int pixelSize, Point2D p, RGBColor c) {
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    label.setFillColor(sf::Color(c.r, c.g, c.b));

    const Point2D worldPosition = Transform(p);
    label.setPosition({worldPosition.x, worldPosition.y});
    label.setRotation(sf::radians(mContextRotation));

    mWindow->draw(label);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a line of text centred on a given point.
 *
 * SFML positions a text object by its top-left corner, so the local bounds are
 * measured and the origin is shifted by half their size before the text is
 * moved to p. Without this the text would sit down and to the right of p.
 *
 * @param text The string to render.
 * @param pixelSize Height of the glyphs in pixels.
 * @param p The point that should sit at the centre of the text.
 * @param c The fill color of the glyphs.
 * @return None. The text is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawCenteredText(const std::string& text, int pixelSize, Point2D p, RGBColor c) {
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    label.setFillColor(sf::Color(c.r, c.g, c.b));

    // Shift by half the bounds so p lands on the centre of the glyphs.
    const sf::FloatRect bounds = label.getLocalBounds();
    label.setOrigin(
        {bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});

    const Point2D worldPosition = Transform(p);
    label.setPosition({worldPosition.x, worldPosition.y});
    label.setRotation(sf::radians(mContextRotation));

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
 * @param p The centre of the circle in local coordinates.
 * @param radius The radius of the circle in pixels.
 * @param c The fill color of the circle.
 * @return None. The circle is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawCircle(Point2D p, float radius, RGBColor c) {
    sf::CircleShape circle(radius);
    circle.setFillColor(sf::Color(c.r, c.g, c.b));
    circle.setOrigin({radius, radius});

    const Point2D worldPosition = Transform(p);
    circle.setPosition({worldPosition.x, worldPosition.y});

    // A circle looks identical at every angle, so it is not rotated.
    mWindow->draw(circle);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a solid rectangle.
 *
 * The rectangle is placed at its transformed top-left corner and then rotated
 * about that corner, so the local geometry stays aligned with the object.
 *
 * @param r The rectangle to fill, in local coordinates.
 * @param c The fill color of the rectangle.
 * @return None. The rectangle is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawRect(Rect r, RGBColor c) {
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color(c.r, c.g, c.b));

    const Point2D worldPosition = Transform(r.topLeft);
    rect.setPosition({worldPosition.x, worldPosition.y});
    rect.setRotation(sf::radians(mContextRotation));

    mWindow->draw(rect);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the outline of a rectangle with its interior left empty.
 *
 * The fill is made transparent and the colour goes on the outline instead, so
 * only the border pixels are actually painted.
 *
 * @param r The rectangle to outline, in local coordinates.
 * @param width The thickness of the border in pixels.
 * @param c The color of the border.
 * @return None. The outline is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::FrameRect(Rect r, float width, RGBColor c) {
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(width);
    rect.setOutlineColor(sf::Color(c.r, c.g, c.b));

    const Point2D worldPosition = Transform(r.topLeft);
    rect.setPosition({worldPosition.x, worldPosition.y});
    rect.setRotation(sf::radians(mContextRotation));

    mWindow->draw(rect);
}

//-----------------------------------------------------------------------
/**
 * @brief Draws a line segment of a given width between two points.
 *
 * SFML has no line primitive, so the segment is built as a four-point convex
 * quad. The two corners at each end are offset by half the width along the
 * normal, which keeps the quad spanning exactly from "from" to "to". Both ends
 * go through the context transform, so the line follows the object's rotation.
 *
 * A zero-length segment has no direction to offset along, so the normal falls
 * back to zero and the result collapses to a single point.
 *
 * @param from The starting point of the segment, in local coordinates.
 * @param to The ending point of the segment, in local coordinates.
 * @param width The thickness of the line in pixels.
 * @param c The color of the line.
 * @return None. The line is drawn immediately onto the window.
 */
//-----------------------------------------------------------------------
void DrawContext::DrawLine(Point2D from, Point2D to, float width, RGBColor c) {
    from = Transform(from);
    to = Transform(to);

    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);

    // Half the width, perpendicular to the segment, is the only offset needed.
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
 * @return The horizontal size of the render window.
 */
//-----------------------------------------------------------------------
int DrawContext::GetWindowWidth() { return static_cast<int>(mWindow->getSize().x); }

//-----------------------------------------------------------------------
/**
 * @brief Reports the height of the window in pixels.
 * @return The vertical size of the render window.
 */
//-----------------------------------------------------------------------
int DrawContext::GetWindowHeight() { return static_cast<int>(mWindow->getSize().y); }

}  // namespace CMPUT350
