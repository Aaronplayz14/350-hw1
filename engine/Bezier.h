#ifndef BEZIER_H
#define BEZIER_H

#include <vector>
#include "MathUtil.h"

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Evaluates cubic Bezier curves made of one or more connected segments.
 *
 * A curve is defined by 3n + 1 control points, giving n cubic segments that
 * join end to end. The parameter t counts whole segments rather than a single
 * [0, 1] range, so with n segments 0 <= t <= n. Because segment k spans
 * t in [k, k+1], asking for t = 2.5 evaluates halfway along the third segment.
 *
 * The curve can be used two ways: statically, by passing the control points to
 * the static overloads, or by constructing a Bezier object once and reusing it
 * for many lookups.
 */
//-----------------------------------------------------------------------
class Bezier {
public:
	/**
	 * @brief Stores the control points for later queries.
	 * @param pts Control points; the count must be 3n + 1.
	 */
	Bezier(const std::vector<Point2D> &pts);

	/// @brief Point on the curve at parameter t, where 0 <= t <= segments.
	Point2D GetPoint(float t) const;
	/// @brief Tangent (un-normalised derivative) at parameter t.
	Point2D GetSlope(float t) const;

	/// @brief Static point lookup; does not require a Bezier instance.
	static Point2D GetPoint(const std::vector<Point2D> &pts, float t);
	/// @brief Static tangent lookup; does not require a Bezier instance.
	static Point2D GetSlope(const std::vector<Point2D> &pts, float t);

private:
    std::vector<Point2D> bezPts; // The control points.
    size_t seg;                  // Number of cubic segments (3n + 1 points).
};

}  // namespace CMPUT350

#endif // BEZIER_H
