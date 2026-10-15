#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <cstdint>
#include <memory>
#include <string>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>

namespace CMPUT350 {

struct RGBColor {
    uint8_t r, g, b;

    RGBColor(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
};

namespace Colors {

const RGBColor red(255, 0, 0);
const RGBColor green(0, 255, 0);
const RGBColor blue(0, 0, 255);
const RGBColor yellow(255, 255, 0);
const RGBColor cyan(0, 255, 255);
const RGBColor magenta(255, 0, 255);
const RGBColor white(255, 255, 255);
const RGBColor black(0, 0, 0);
const RGBColor gray(100, 100, 100);
const RGBColor grey(200, 200, 200);

}  // namespace Colors

class GameEngine;

class DrawContext {
public:
    DrawContext(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font);

    void DrawText(const std::string &text, int pixelSize, Point2D p, RGBColor c);
    void DrawCenteredText(const std::string &text, int pixelSize, Point2D p, RGBColor c);
    void DrawCircle(Point2D p, float radius, RGBColor c);
    void DrawRect(Rect r, RGBColor c);
    void FrameRect(Rect r, float width, RGBColor c);
    void DrawLine(Point2D from, Point2D to, float width, RGBColor c);

    int GetWindowWidth();
    int GetWindowHeight();

private:
    // GameEngine sets the transform before asking an object to render itself.
    // Kept private so individual game objects cannot change it mid-draw.
    friend class GameEngine;

    // Where this object's own origin (0, 0) sits in the world.
    Point2D mContextOffset{0.0f, 0.0f};

    // Extra rotation on top of the offset, in radians.
    float mContextRotation = 0.0f;

    void SetContextOffset(Point2D offset);
    void SetContextRotation(float rotation);

    // Conversions between the object's local space and world space.
    Point2D Transform(Point2D p) const;
    Point2D ReverseTransform(Point2D p) const;

    // Smallest upright world box that still contains a rotated local rect.
    Rect Transform(Rect r) const;

    std::shared_ptr<sf::RenderWindow> mWindow;
    std::shared_ptr<sf::Font> mFont;
};

}  // namespace CMPUT350

#endif  // RENDERTARGET_H
