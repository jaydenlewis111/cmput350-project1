#include "CollisionObject.h"
#include <array>
#include <limits>
namespace CMPUT350 {
namespace {
constexpr float epsilon = 0.00001f;
// Transform a local directed segment into screen space.
Line WorldLine(Line line, const CollisionObject &object) {
    return {ToWorld(line.p1, object.GetLocation(), object.GetRotation()),
            ToWorld(line.p2, object.GetLocation(), object.GetRotation())};
}
Circle WorldCircle(Circle circle, const CollisionObject &object) {
    circle.center = ToWorld(circle.center, object.GetLocation(), object.GetRotation());
    return circle;
}
// Parameter of the first segment/circle contact, including an inside start.
bool LineCircle(Line line, Circle circle, float &t) {
    const auto d = line.p2 - line.p1, f = line.p1 - circle.center;
    const float c = f.Dot(f) - circle.radius * circle.radius;
    if (c <= epsilon) { t = 0; return true; }
    const float a = d.Dot(d), b = 2 * f.Dot(d);
    if (a <= epsilon) return false;
    const float discriminant = b*b - 4*a*c;
    if (discriminant < -epsilon) return false;
    t = (-b - std::sqrt(std::max(0.f, discriminant))) / (2*a);
    if (t < -epsilon || t > 1 + epsilon) return false;
    t = std::clamp(t, 0.f, 1.f);
    return true;
}
// Clip a local directed segment against a local AABB with the slab algorithm.
bool LineRect(Line line, Rect rect, float &t) {
    float enter = 0, leave = 1;
    const float starts[] = {line.p1.x, line.p1.y};
    const float deltas[] = {line.p2.x-line.p1.x, line.p2.y-line.p1.y};
    const float minima[] = {rect.topLeft.x, rect.topLeft.y};
    const float maxima[] = {rect.topLeft.x+rect.width, rect.topLeft.y+rect.height};
    for (int axis = 0; axis < 2; ++axis) {
        if (std::abs(deltas[axis]) < epsilon) {
            if (starts[axis] < minima[axis]-epsilon || starts[axis] > maxima[axis]+epsilon) return false;
        } else {
            float a = (minima[axis]-starts[axis])/deltas[axis];
            float b = (maxima[axis]-starts[axis])/deltas[axis];
            if (a > b) std::swap(a, b);
            enter = std::max(enter, a); leave = std::min(leave, b);
            if (enter > leave + epsilon) return false;
        }
    }
    t = std::clamp(enter, 0.f, 1.f);
    return true;
}
// First directed contact against any component, represented in world space.
bool LineShape(Line line, const Shape &shape, const CollisionObject &object, Point2D &point, float &t) {
    if (shape.t == ShapeType::kLine) {
        if (!line.Crosses(WorldLine(shape.shape.line, object), point)) return false;
        const auto d = line.p2-line.p1;
        t = d.Dot(d) > 0 ? (point-line.p1).Dot(d)/d.Dot(d) : 0;
        return true;
    }
    if (shape.t == ShapeType::kCircle) {
        if (!LineCircle(line, WorldCircle(shape.shape.circle, object), t)) return false;
    } else {
        Line local(ToLocal(line.p1, object.GetLocation(), object.GetRotation()),
                   ToLocal(line.p2, object.GetLocation(), object.GetRotation()));
        if (!LineRect(local, shape.shape.rect, t)) return false;
    }
    point = line.p1 + (line.p2-line.p1)*t;
    return true;
}
// Compare a world circle to an oriented rectangle by undoing its transform.
bool CircleRect(Circle circle, Rect rect, const CollisionObject &object) {
    const auto p = ToLocal(circle.center, object.GetLocation(), object.GetRotation());
    const Point2D closest(std::clamp(p.x, rect.topLeft.x, rect.topLeft.x+rect.width),
                          std::clamp(p.y, rect.topLeft.y, rect.topLeft.y+rect.height));
    return closest.Distance(p) <= circle.radius + epsilon;
}
std::array<Point2D,4> Corners(Rect r, const CollisionObject &o) {
    return {ToWorld(r.topLeft,o.GetLocation(),o.GetRotation()),
            ToWorld(r.topLeft+Point2D(r.width,0),o.GetLocation(),o.GetRotation()),
            ToWorld(r.topLeft+Point2D(r.width,r.height),o.GetLocation(),o.GetRotation()),
            ToWorld(r.topLeft+Point2D(0,r.height),o.GetLocation(),o.GetRotation())};
}
// Separating-axis test: oriented rectangles overlap on both axes of both boxes.
bool RectRect(Rect a, const CollisionObject &oa, Rect b, const CollisionObject &ob) {
    const auto ca = Corners(a, oa), cb = Corners(b, ob);
    const std::array<Vector2D,4> axes = {Rotate({1,0},oa.GetRotation()), Rotate({0,1},oa.GetRotation()),
                                        Rotate({1,0},ob.GetRotation()), Rotate({0,1},ob.GetRotation())};
    for (auto axis : axes) {
        float minA=ca[0].Dot(axis),maxA=minA,minB=cb[0].Dot(axis),maxB=minB;
        for (int i=1;i<4;++i) {
            minA=std::min(minA,ca[i].Dot(axis)); maxA=std::max(maxA,ca[i].Dot(axis));
            minB=std::min(minB,cb[i].Dot(axis)); maxB=std::max(maxB,cb[i].Dot(axis));
        }
        if (maxA < minB-epsilon || maxB < minA-epsilon) return false;
    }
    return true;
}
bool SolidShapes(const Shape &a, const CollisionObject &oa, const Shape &b, const CollisionObject &ob) {
    if (a.t == ShapeType::kCircle && b.t == ShapeType::kCircle) {
        auto ca=WorldCircle(a.shape.circle,oa), cb=WorldCircle(b.shape.circle,ob);
        return ca.center.Distance(cb.center) <= ca.radius+cb.radius+epsilon;
    }
    if (a.t == ShapeType::kCircle) return CircleRect(WorldCircle(a.shape.circle,oa),b.shape.rect,ob);
    if (b.t == ShapeType::kCircle) return CircleRect(WorldCircle(b.shape.circle,ob),a.shape.rect,oa);
    return RectRect(a.shape.rect,oa,b.shape.rect,ob);
}
}
CollisionResult FindCollision(CollisionObject &a, CollisionObject &b) {
    CollisionResult result;
    const Rect boundsA=WorldBounds(a.GetBounds(),a.GetLocation(),a.GetRotation());
    if (!boundsA.Overlaps(WorldBounds(b.GetBounds(),b.GetLocation(),b.GetRotation()))) return result;
    float bestA=std::numeric_limits<float>::infinity(),bestB=bestA;
    for (const auto &sa : a.GetShapes()) {
        for (const auto &sb : b.GetShapes()) {
            if (sa.t != ShapeType::kLine && sb.t != ShapeType::kLine) {
                result.hit = SolidShapes(sa,a,sb,b) || result.hit;
                continue;
            }
            Point2D pa,pb; float ta=0,tb=0;
            const bool lineA=sa.t==ShapeType::kLine,lineB=sb.t==ShapeType::kLine;
            if (lineA && !LineShape(WorldLine(sa.shape.line,a),sb,b,pa,ta)) continue;
            if (lineB && !LineShape(WorldLine(sb.shape.line,b),sa,a,pb,tb)) continue;
            if (!lineA) { pa=pb; ta=tb; }
            if (!lineB) { pb=pa; tb=ta; }
            result.hit = true;
            if (ta < bestA) { bestA=ta; result.pointA=ToLocal(pa,a.GetLocation(),a.GetRotation()); }
            if (tb < bestB) { bestB=tb; result.pointB=ToLocal(pb,b.GetLocation(),b.GetRotation()); }
        }
    }
    return result;
}
}
