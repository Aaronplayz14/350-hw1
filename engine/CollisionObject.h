#ifndef COLLISION_OBJECT_H
#define COLLISION_OBJECT_H

#include <memory>
#include <vector>

#include "GraphicsObject.h"
#include "MathUtil.h"

namespace CMPUT350 {

/// @brief The kinds of primitive a collision shape can be.
enum class ShapeType { kLine, kCircle, kRect };

//-----------------------------------------------------------------------
/**
 * @brief Holds whichever primitive is stored in a Shape.
 *
 * Only the member matching the Shape's ShapeType is valid at any time.
 */
//-----------------------------------------------------------------------
union ShapeUnion {
    ShapeUnion(Line l) : line(l) {}
    ShapeUnion(Circle c) : circle(c) {}
    ShapeUnion(Rect r) : rect(r) {}
    Line line;
    Circle circle;
    Rect rect;  // Axis-aligned rectangle
};

//-----------------------------------------------------------------------
/**
 * @brief One primitive making up an object's collision volume.
 *
 * The union holds the geometry and the tag records which member is live. The
 * constructors set both so a Shape is never left half-initialised.
 */
//-----------------------------------------------------------------------
struct Shape {
    Shape(const Line& l) : shape(l), t(ShapeType::kLine) {}
    Shape(const Circle& c) : shape(c), t(ShapeType::kCircle) {}
    Shape(const Rect& r) : shape(r), t(ShapeType::kRect) {}
    ShapeUnion shape;
    ShapeType t;
};

//-----------------------------------------------------------------------
/**
 * @brief Base class for anything that can take part in a collision.
 *
 * A collision object reports a bounding box for cheap broad-phase rejection
 * and a list of local-space shapes for the exact test. The engine transforms
 * those shapes by the object's location and rotation before comparing them, so
 * an object always describes its collision volume in its own coordinates.
 *
 * When two objects overlap, the engine calls CollisionEnter on both of them
 * so each can react independently. If the contact has a single well-defined
 * point (a line crossing something) the overload that carries the point is
 * used; otherwise the object receives the two-argument version with a point
 * expressed in its own local space.
 */
//-----------------------------------------------------------------------
class CollisionObject : public GraphicsObject {
public:
    /// @brief Called when this object collides with another; no point given.
    virtual void CollisionEnter(const std::shared_ptr<CollisionObject>& obj) = 0;
    /// @brief Called when the collision has a single contact point in local space.
    virtual void CollisionEnter(const std::shared_ptr<CollisionObject>& obj,
                                const Point2D& collisionPoint) {
        CollisionEnter(obj);
    }
    /// @brief World-space bounding box used for the cheap overlap test.
    virtual const Rect& GetBounds() = 0;
    /// @brief Local-space primitives that define the exact collision volume.
    virtual const std::vector<Shape>& GetShapes() = 0;
};

}  // namespace CMPUT350

#endif
