#include "DrawContext.h"

#include <cmath>

namespace CMPUT350 {

DrawContext::DrawContext(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font)
    : mWindow(window), mFont(font) {}

void DrawContext::DrawText(const std::string&text, int pixelSize, Point2D p, RGBColor c)
{
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    label.setFillColor(sf::Color(c.r, c.g, c.b));
    label.setPosition({p.x, p.y});

    mWindow->draw(label);
}

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

void DrawContext::DrawCircle(Point2D p, float radius, RGBColor c)
{
    sf::CircleShape circle(radius);
    circle.setFillColor(sf::Color(c.r, c.g, c.b));
    circle.setOrigin({radius, radius});
    circle.setPosition({p.x, p.y});

    mWindow->draw(circle);
}

void DrawContext::DrawRect(Rect r, RGBColor c)
{
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color(c.r, c.g, c.b));
    rect.setPosition({r.topLeft.x, r.topLeft.y});

    mWindow->draw(rect);
}

void DrawContext::FrameRect(Rect r, float width, RGBColor c)
{
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(width);
    rect.setOutlineColor(sf::Color(c.r, c.g, c.b));
    rect.setPosition({r.topLeft.x, r.topLeft.y});

    mWindow->draw(rect);
}

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

int DrawContext::GetWindowWidth() { return mWindow->getSize().x; }

int DrawContext::GetWindowHeight() { return mWindow->getSize().y; }

}  // namespace CMPUT350