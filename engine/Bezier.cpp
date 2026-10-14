#include "Bezier.h"
#include <cassert>
#include <cmath>

namespace CMPUT350 {

Bezier::Bezier(const std::vector<Point2D> &pts)
{
    assert(pts.size() != 0 && (pts.size() - 1) % 3 == 0);
    seg = (pts.size() - 1) / 3;
    bezPts = pts;
}

Point2D Bezier::GetPoint(float t) const
{
    assert(t >= 0 && t <= seg);

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

Point2D Bezier::GetSlope(float t) const
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

Point2D Bezier::GetSlope(const std::vector<Point2D> &pts, float t)
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