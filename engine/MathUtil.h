#ifndef MATHUTIL_H
#define MATHUTIL_H

#include <cmath>
#include <iostream>

namespace CMPUT350 {

struct Point2D {
    float x, y;
    Point2D(float x = 0, float y = 0) : x(x), y(y) {}
    double Distance(const Point2D &other) const {
        float xDif = other.x - x;
        float yDif = other.y - y;
        double d = sqrt(xDif * xDif + yDif * yDif);
        return d;
    }
    Point2D operator+(const Point2D &other) const {
        return Point2D(x + other.x, y + other.y);
    }
    Point2D operator+(const float &other) const {
        return Point2D(x + other, y + other);
    }
    Point2D operator-(const Point2D &other) const {
        return Point2D(x - other.x, y - other.y);
    }
    Point2D operator-(const float &other) const {
        return Point2D(x - other, y - other);
    }
    Point2D operator*(const float &scalar) const {
        return Point2D(x * scalar, y * scalar);
    }
    Point2D &operator+=(const float &scalar) {
        x += scalar;
        y += scalar;
        return *this;
    }
    Point2D &operator+=(const Point2D &other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    Point2D &operator-=(const Point2D &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    bool operator==(const Point2D &other) const {
        return x == other.x && y == other.y;
    }
    Point2D &operator*=(const float &scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    Point2D &operator/=(const float &scalar) {
        if(scalar == 0){
            std::cerr << "WARNING: Cannot divide by 0, setting Point2D to origin\n";
            x = 0;
            y = 0;
        } else {
            x /= scalar;
            y /= scalar;
        }
        
        return *this;
    }
    float operator*(const Point2D &other) const {
        return x*other.x + y*other.y;
    }
    float Dot(Point2D b) const {
        return x*b.x + y*b.y;
    }
    float Cross(Point2D b) const {
        return x*b.y - y*b.x;
    }
    static float Dot(Point2D a, Point2D b) {
        return a.x*b.x + a.y*b.y;
    }
    static float Cross(Point2D a, Point2D b) { 
        return a.x*b.y - a.y*b.x;
    }
    void Normalize() {
        float mag = static_cast<float>(this->Distance(Point2D()));
        if (mag == 0){
            std::cerr << "WARNING: Point2D at origin, cannot normalize\n";
            x = 0;
            y = 0;
        } else {
            x /= mag;
            y /= mag;
        }
    }
};

static std::ostream &operator<<(std::ostream &os, const Point2D &p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

static Point2D operator*(float number, const Point2D &rhs) {
    return Point2D(number*rhs.x, number*rhs.y);
}

struct Line {
    Point2D p1, p2;

    Line(Point2D p1 = {0, 0}, Point2D p2 = {0, 0}) : p1(p1), p2(p2) {}
    Line(float x1, float y1, float x2, float y2) : p1(x1, y1), p2(x2, y2) {}
    float Length() const {
        return static_cast<float>(p1.Distance(p2));
    }
    Point2D ClosestPoint(const Point2D &p) const {
        Point2D ab = p2 - p1;
        Point2D ac = p - p1;
        Point2D bc = p - p2;

        if (Point2D::Dot(ab, ac) <= 0) {
            return p1;
        } else if (Point2D::Dot(ab, bc) >= 0) {
            return p2;
        }

        float x = Point2D::Dot(ab, ac) / this->Length();
        return p1 + (ab * (x / this->Length()));
    }
    bool Crosses(Line other, Point2D &crossingPoint) const {
        Point2D ab = p2 - p1;
        Point2D xy = other.p2 - other.p1;

        if(Point2D::Cross(ab, xy) == 0 || Point2D::Cross(xy, ab) == 0){
            return false;
        }

        float t = Point2D::Cross((other.p1 - p1), xy) / Point2D::Cross(ab, xy);
        float u = Point2D::Cross((p1 - other.p1), ab) / Point2D::Cross(xy, ab);

        if (t >= 0 && t<= 1 && u >= 0 && u <= 1) {
            crossingPoint = p1 + (ab * t);
            return true;
        } else {
            return false;
        }
    }
};

static std::ostream &operator<<(std::ostream &os, const Line &l) {
    return os << "(" << l.p1 << ", " << l.p2 << ")";
}

struct Circle {
    Point2D center;
    float radius;

    Circle(Point2D c = {0, 0}, float r = 0) : center(c), radius(r) {}

    Circle(float x, float y, float r) : center(x, y), radius(r) {}
};

struct Rect {
    Point2D topLeft;
    float width, height;

    Rect(float left, float top, float width, float height)
        : topLeft(Point2D(left, top)), width(width), height(height) {}

    Rect(Point2D tl = {0, 0}, int w = 0, int h = 0) : topLeft(tl), width(w), height(h) {}

    // Creates bounding box around p1 and p2 with positive width/height
    Rect(Point2D p1, Point2D p2)
        : topLeft(std::min(p1.x, p2.x), std::min(p1.y, p2.y)),
          width(fabs(p1.x - p2.x)),
          height(fabs(p1.y - p2.y)) {}

    Rect(Point2D center, float radius)
        : topLeft(center.x - radius, center.y - radius), width(2 * radius), height(2 * radius) {}

    Rect &operator|=(const Rect &other) {
        float xMin, xMax, yMin, yMax;
        xMin = std::min(topLeft.x, other.topLeft.x);
        yMin = std::min(topLeft.y, other.topLeft.y);
        xMax = std::max(topLeft.x + width, other.topLeft.x + other.width);
        yMax = std::max(topLeft.y + height, other.topLeft.y + other.height);

        topLeft.x = xMin;
        topLeft.y = yMin;
        width = xMax - xMin;
        height = yMax - yMin;

        return *this;
    }
    Rect &operator|=(const Point2D &other) {
        float xMin, xMax, yMin, yMax;
        xMin = std::min(topLeft.x, other.x);
        yMin = std::min(topLeft.y, other.y);
        xMax = std::max(topLeft.x + width, other.x);
        yMax = std::max(topLeft.y + height, other.y);

        topLeft.x = xMin;
        topLeft.y = yMin;
        width = xMax - xMin;
        height = yMax - yMin;
        return *this;
    }
    Rect &operator|=(const Line &other) {
        float xMin, xMax, yMin, yMax;
        xMin = std::min({topLeft.x, other.p1.x, other.p2.x});
        yMin = std::min({topLeft.y, other.p1.y, other.p2.y});
        xMax = std::max({topLeft.x + width, other.p1.x, other.p2.x});
        yMax = std::max({topLeft.y + height, other.p1.y, other.p2.y});

        topLeft.x = xMin;
        topLeft.y = yMin;
        width = xMax - xMin;
        height = yMax - yMin;
        return *this;
    }
    Rect &operator&=(const Rect &other) {
        float xLB, xUB, yLB, yUB;
        xLB = std::max(topLeft.x, other.topLeft.x);
        yLB = std::max(topLeft.y, other.topLeft.y);
        xUB = std::min(topLeft.x + width, other.topLeft.x + other.width);
        yUB = std::min(topLeft.y + height, other.topLeft.y + other.height);

        if (xLB <= xUB && yLB <= yUB) {
            topLeft.x = xLB;
            topLeft.y = yLB;
            width = xUB - xLB;
            height = yUB - yLB;
        } else {
            topLeft.x = NAN;
            topLeft.y = NAN;
            width = NAN;
            height = NAN;
        }

        return *this;
    }
    Rect &operator+=(const Point2D &other) {
        topLeft += other;
        return *this;
    }
    Rect operator+(const Point2D &other) const {
        return Rect(topLeft + other, width, height);
    }
    void Inset(float inset) {
        float newWidth = std::max(width - inset * 2, 0.0f);
        float newHeight = std::max(height - inset * 2, 0.0f);
        topLeft += Point2D((width - newWidth) / 2, (height - newHeight) / 2);
        width = newWidth;
        height = newHeight;
    }
    bool IsInside(const Point2D &p) const {
        if (p.x >= topLeft.x && p.x <= topLeft.x + width && p.y >= topLeft.y && p.y <= topLeft.y + height) {
            return true;
        }
        return false;
    }
};

static std::ostream &operator<<(std::ostream &os, const Rect &l) {
    return os << "(" << l.topLeft << ", W: " << l.width << ", H: " << l.height << ")";
}

}  // namespace CMPUT350

#endif  // MATHUTIL_H
