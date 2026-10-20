#ifndef COLLISION_OBJECT_H
#define COLLISION_OBJECT_H
#include "GraphicsObject.h"
#include <memory>
#include <optional>
#include <vector>
namespace CMPUT350 {
enum class ShapeType { kLine, kCircle, kRect };
union ShapeUnion {
    ShapeUnion(Line l) : line(l) {}
    ShapeUnion(Circle c) : circle(c) {}
    ShapeUnion(Rect r) : rect(r) {}
    Line line;
    Circle circle;
    Rect rect;
};
struct Shape {
    Shape(const Line &l) : shape(l), t(ShapeType::kLine) {}
    Shape(const Circle &c) : shape(c), t(ShapeType::kCircle) {}
    Shape(const Rect &r) : shape(r), t(ShapeType::kRect) {}
    ShapeUnion shape;
    ShapeType t;
};
class CollisionObject : public GraphicsObject {
public:
    // Notify once per overlapping object pair. Points, when available, are local.
    virtual void CollisionEnter(const std::shared_ptr<CollisionObject> &obj) = 0;
    virtual void CollisionEnter(const std::shared_ptr<CollisionObject> &obj, const Point2D &) { CollisionEnter(obj); }
    // Return local-space broad-phase bounds and the union of precise shapes.
    virtual const Rect &GetBounds() = 0;
    virtual const std::vector<Shape> &GetShapes() = 0;
};
struct CollisionResult {
    bool hit = false;
    std::optional<Point2D> pointA, pointB;
};
// Test world AABBs, then all component pairs. Directed lines produce their first
// intersection; result points are converted back into each object's local space.
CollisionResult FindCollision(CollisionObject &a, CollisionObject &b);
}
#endif
