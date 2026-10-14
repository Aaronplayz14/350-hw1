#ifndef BEZIER_H
#define BEZIER_H

#include <vector>
#include "MathUtil.h"

namespace CMPUT350 {

class Bezier {
public:
	Bezier(const std::vector<Point2D> &pts);
	
	Point2D GetPoint(float t) const;
	Point2D GetSlope(float t) const;
	
	static Point2D GetPoint(const std::vector<Point2D> &pts, float t);
	static Point2D GetSlope(const std::vector<Point2D> &pts, float t);

private:
    std::vector<Point2D> bezPts;
    size_t seg;
};

}  // namespace CMPUT350

#endif  // BEZIER_H