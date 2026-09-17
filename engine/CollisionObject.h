#ifndef COLLISION_OBJECT_H
#define COLLISION_OBJECT_H

#include <memory>

#include "GraphicsObject.h"
#include "MathUtil.h"

namespace CMPUT350 {

class CollisionObject : public GraphicsObject {
public:
    virtual void CollisionEnter(const std::shared_ptr<CollisionObject> &obj) = 0;
    virtual const Rect &GetBounds() = 0;
};

}  // namespace CMPUT350

#endif
