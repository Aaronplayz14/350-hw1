#include "Bezier.h"
#include <cassert>
#include <cmath>

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Stores the control points that define the curve.
 * @param pts Control points; the count must satisfy (size - 1) % 3 == 0.
 */
//-----------------------------------------------------------------------
Bezier::Bezier(const std::vector<Point2D> &pts)
{
    assert(pts.size() != 0 && (pts.size() - 1) % 3 == 0);
    seg = (pts.size() - 1) / 3;
    bezPts = pts;
}

//-----------------------------------------------------------------------
/**
 * @brief Evaluates the curve at parameter t using the stored control points.
 *
 * The whole-number part of t selects the cubic segment and the fractional part
 * becomes the local parameter within it, after which the standard cubic
 * Bernstein form is used.
 *
 * @param t Curve parameter, 0 <= t <= seg.
 * @return The point on the curve.
 */
//-----------------------------------------------------------------------
Point2D Bezier::GetPoint(float t) const
{
    assert(t >= 0 && t <= seg);

    // Split t into the segment index (tFloor) and the offset within it.
    float tFloor;
    if (t == seg)
        tFloor = t - 1;
    else
        tFloor = std::floor(t);
    float tSeg = t - tFloor;
    float tSegI = 1 - tSeg;

    float x = (tSegI * tSegI * tSegI * bezPts[tFloor * 3].x) + 
              (3 * tSegI * tSegI * tSeg * bezPts[1 + tFloor * 3].x) +
              (3 * tSegI * tSeg * tSeg * bezPts[2 + tFloor * 3].x) +
              (tSeg * tSeg * tSeg * bezPts[3 + tFloor * 3].x);
    
    float y = (tSegI * tSegI * tSegI * bezPts[tFloor * 3].y) + 
              (3 * tSegI * tSegI * tSeg * bezPts[1 + tFloor * 3].y) +
              (3 * tSegI * tSeg * tSeg * bezPts[2 + tFloor * 3].y) +
              (tSeg * tSeg * tSeg * bezPts[3 + tFloor * 3].y);

    return Point2D(x, y);
}

//-----------------------------------------------------------------------
/**
 * @brief Returns the tangent vector of the curve at parameter t.
 *
 * The result is the derivative of the cubic Bernstein form (an un-normalised
 * direction); callers that only need a heading use atan2 on it.
 *
 * @param t Curve parameter, 0 <= t <= seg.
 * @return The tangent vector at t.
 */
//-----------------------------------------------------------------------
CMPUT350::Vector2D Bezier::GetSlope(float t) const
{
    assert(t >= 0 && t <= seg);

    float tFloor;
    if (t == seg)
        tFloor = t - 1;
    else
        tFloor = std::floor(t);
    float tSeg = t - tFloor;
    float tSegI = 1 - tSeg;

    float dy = (3 * tSegI * tSegI * (bezPts[1 + tFloor * 3].y - bezPts[tFloor * 3].y)) +
               (6 * tSegI * tSeg * (bezPts[2 + tFloor * 3].y - bezPts[1 + tFloor * 3].y)) +
               (3 * tSeg * tSeg * (bezPts[3 + tFloor * 3].y - bezPts[2 + tFloor * 3].y));

    float dx = (3 * tSegI * tSegI * (bezPts[1 + tFloor * 3].x - bezPts[tFloor * 3].x)) +
               (6 * tSegI * tSeg * (bezPts[2 + tFloor * 3].x - bezPts[1 + tFloor * 3].x)) +
               (3 * tSeg * tSeg * (bezPts[3 + tFloor * 3].x - bezPts[2 + tFloor * 3].x));

    return Point2D(dx, dy);
}

//-----------------------------------------------------------------------
/**
 * @brief Evaluates a curve from an explicit control-point list.
 * @param pts Control points; (size - 1) must be divisible by 3.
 * @param t Curve parameter, 0 <= t <= number of segments.
 * @return The point on the curve.
 */
//-----------------------------------------------------------------------
Point2D Bezier::GetPoint(const std::vector<Point2D> &pts, float t)
{
    assert(pts.size() != 0 && (pts.size() - 1) % 3 == 0);
    size_t seg = (pts.size() - 1) / 3;

    assert(t >= 0 && t <= seg);

    float tFloor;
    if (t == seg)
        tFloor = t - 1;
    else
        tFloor = std::floor(t);
    float tSeg = t - tFloor;
    float tSegI = 1 - tSeg;

    float x = (tSegI * tSegI * tSegI * pts[tFloor * 3].x) + 
              (3 * tSegI * tSegI * tSeg * pts[1 + tFloor * 3].x) +
              (3 * tSegI * tSeg * tSeg * pts[2 + tFloor * 3].x) +
              (tSeg * tSeg * tSeg * pts[3 + tFloor * 3].x);
    
    float y = (tSegI * tSegI * tSegI * pts[tFloor * 3].y) + 
              (3 * tSegI * tSegI * tSeg * pts[1 + tFloor * 3].y) +
              (3 * tSegI * tSeg * tSeg * pts[2 + tFloor * 3].y) +
              (tSeg * tSeg * tSeg * pts[3 + tFloor * 3].y);

    return Point2D(x, y);

}

//-----------------------------------------------------------------------
/**
 * @brief Returns the tangent of a curve from an explicit control-point list.
 * @param pts Control points; (size - 1) must be divisible by 3.
 * @param t Curve parameter, 0 <= t <= number of segments.
 * @return The tangent vector at t.
 */
//-----------------------------------------------------------------------
CMPUT350::Vector2D Bezier::GetSlope(const std::vector<Point2D> &pts, float t)
{
    assert(pts.size() != 0 && (pts.size() - 1) % 3 == 0);
    size_t seg = (pts.size() - 1) / 3;
    
    assert(t >= 0 && t <= seg);

    float tFloor;
    if (t == seg)
        tFloor = t - 1;
    else
        tFloor = std::floor(t);
    float tSeg = t - tFloor;
    float tSegI = 1 - tSeg;

    float dy = (3 * tSegI * tSegI * (pts[1 + tFloor * 3].y - pts[tFloor * 3].y)) +
               (6 * tSegI * tSeg * (pts[2 + tFloor * 3].y - pts[1 + tFloor * 3].y)) +
               (3 * tSeg * tSeg * (pts[3 + tFloor * 3].y - pts[2 + tFloor * 3].y));

    float dx = (3 * tSegI * tSegI * (pts[1 + tFloor * 3].x - pts[tFloor * 3].x)) +
               (6 * tSegI * tSeg * (pts[2 + tFloor * 3].x - pts[1 + tFloor * 3].x)) +
               (3 * tSeg * tSeg * (pts[3 + tFloor * 3].x - pts[2 + tFloor * 3].x));

    return Point2D(dx, dy);
}

}  // namespace CMPUT350