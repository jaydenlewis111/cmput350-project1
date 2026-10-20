#ifndef MATHUTIL_H
#define MATHUTIL_H
#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdexcept>
namespace CMPUT350 {

// Pixel vectors: +x points right, +y down. Scalar operations affect both axes.
// Assignment operators mutate and return this point; other operators return values.
struct Point2D {
    float x, y;
    Point2D(float x=0, float y=0) : x(x), y(y) {}
    float Distance(const Point2D &b) const { return std::hypot(x-b.x, y-b.y); }
    Point2D operator+(const Point2D &b) const { return {x+b.x,y+b.y}; }
    Point2D operator+(const float &s) const { return {x+s,y+s}; }
    Point2D operator-(const Point2D &b) const { return {x-b.x,y-b.y}; }
    Point2D operator-(const float &s) const { return {x-s,y-s}; }
    Point2D operator*(const float &s) const { return {x*s,y*s}; }
    Point2D &operator+=(const float &s) { x+=s; y+=s; return *this; }
    Point2D &operator+=(const Point2D &b) { x+=b.x; y+=b.y; return *this; }
    Point2D &operator-=(const Point2D &b) { x-=b.x; y-=b.y; return *this; }
    bool operator==(const Point2D &b) const { return x==b.x && y==b.y; }
    Point2D &operator*=(const int &s) { x*=s; y*=s; return *this; }
    Point2D &operator/=(const int &s) { return *this /= static_cast<float>(s); }
    Point2D &operator/=(float s) {
        if (s==0) throw std::invalid_argument("Division by zero");
        x/=s; y/=s; return *this;
    }
    // Point multiplication returns the dot product.
    float operator*(const Point2D &b) const { return Dot(b); }
    float Dot(Point2D b) const { return x*b.x+y*b.y; }
    float Cross(Point2D b) const { return x*b.y-y*b.x; }
    static float Dot(Point2D a, Point2D b) { return a.Dot(b); }
    static float Cross(Point2D a, Point2D b) { return a.Cross(b); }
    // Convert to unit length; a zero vector remains unchanged.
    void Normalize() { const float n=std::hypot(x,y); if(n>0) *this/=n; }
};
// Stream helpers return the supplied stream; scalar multiplication returns a point.
inline std::ostream &operator<<(std::ostream &os,const Point2D &p) { return os<<'('<<p.x<<", "<<p.y<<')'; }
inline Point2D operator*(float s,const Point2D &p) { return p*s; }
using Vector2D = Point2D;

// Rotate a local vector by radians (positive turns clockwise on screen).
inline Vector2D Rotate(Vector2D p, float radians) {
    const float c = std::cos(radians), s = std::sin(radians);
    return {c*p.x - s*p.y, s*p.x + c*p.y};
}
// Convert positions between an object's local frame and screen coordinates.
inline Point2D ToWorld(Point2D p, Point2D offset, float rotation) { return Rotate(p, rotation) + offset; }
inline Point2D ToLocal(Point2D p, Point2D offset, float rotation) { return Rotate(p - offset, -rotation); }

struct Line {
    Point2D p1,p2;
    Line(Point2D a={},Point2D b={}) : p1(a),p2(b) {}
    Line(float x1,float y1,float x2,float y2) : p1(x1,y1),p2(x2,y2) {}
    // Return segment length or its nearest point to p, including endpoints.
    float Length() const { return p1.Distance(p2); }
    Point2D ClosestPoint(const Point2D &p) const {
        const auto d=p2-p1; const float n=d.Dot(d);
        return n==0 ? p1 : p1+d*std::clamp((p-p1).Dot(d)/n,0.f,1.f);
    }
    // Test finite segments; write one shared point on success only.
    bool Crosses(const Line &b,Point2D &crossingPoint) const {
        const auto r=p2-p1,s=b.p2-b.p1,q=b.p1-p1;
        const float divisor=r.Cross(s);
        if(std::abs(divisor)<0.00001f) {
            const float length2 = r.Dot(r);
            if (length2 == 0) {
                if (b.ClosestPoint(p1).Distance(p1) > 0.0001f) return false;
                crossingPoint = p1; return true;
            }
            if (std::abs(q.Cross(r)) > 0.0001f) return false;
            const float a = q.Dot(r) / length2, end = (b.p2-p1).Dot(r) / length2;
            const float first = std::max(0.f, std::min(a, end));
            if (first > std::min(1.f, std::max(a, end))) return false;
            crossingPoint = p1 + r * first; return true;
        }
        const float t=q.Cross(s)/divisor,u=q.Cross(r)/divisor;
        if(t<0 || t>1 || u<0 || u>1) return false;
        crossingPoint=p1+r*t; return true;
    }
};
inline std::ostream &operator<<(std::ostream &os,const Line &l) { return os<<l.p1<<" -> "<<l.p2; }

struct Circle {
    Point2D center; float radius;
    Circle(Point2D c={},float r=0) : center(c),radius(r) {}
    Circle(float x,float y,float r) : center(x,y),radius(r) {}
};
inline std::ostream &operator<<(std::ostream &os, const Circle &c) { return os << c.center << " r=" << c.radius; }

// Axis-aligned pixel bounds. All constructors use x/left before y/top.
struct Rect {
    Point2D topLeft; float width,height;
    Rect(float left,float top,float w,float h) : topLeft(left,top),width(w),height(h) {}
    Rect(Point2D tl={}) : topLeft(tl),width(0),height(0) {}
    Rect(Point2D tl,float w,float h) : topLeft(tl),width(w),height(h) {}
    // Bounds enclosing two points or a circle.
    Rect(Point2D a,Point2D b) : topLeft(std::min(a.x,b.x),std::min(a.y,b.y)),
        width(std::abs(a.x-b.x)),height(std::abs(a.y-b.y)) {}
    Rect(Point2D c,float radius) : topLeft(c-radius),width(2*radius),height(2*radius) {}
    // Expand this rectangle to enclose a rectangle, point, or line.
    Rect &operator|=(const Rect &b) {
        const float right=std::max(topLeft.x+width,b.topLeft.x+b.width);
        const float bottom=std::max(topLeft.y+height,b.topLeft.y+b.height);
        topLeft={std::min(topLeft.x,b.topLeft.x),std::min(topLeft.y,b.topLeft.y)};
        width=right-topLeft.x; height=bottom-topLeft.y; return *this;
    }
    Rect &operator|=(const Point2D &p) { return *this |= Rect(p,0,0); }
    Rect &operator|=(const Line &l) { return *this |= Rect(l.p1,l.p2); }
    // Intersect in place, clamping disjoint dimensions to zero.
    Rect &operator&=(const Rect &b) {
        const float right=std::min(topLeft.x+width,b.topLeft.x+b.width);
        const float bottom=std::min(topLeft.y+height,b.topLeft.y+b.height);
        topLeft={std::max(topLeft.x,b.topLeft.x),std::max(topLeft.y,b.topLeft.y)};
        width=std::max(0.f,right-topLeft.x); height=std::max(0.f,bottom-topLeft.y); return *this;
    }
    // Translate, preserving dimensions.
    Rect &operator+=(const Point2D &p) { topLeft+=p; return *this; }
    Rect operator+(const Point2D &p) const { auto result=*this; return result+=p; }
    // Shrink edges by inset pixels; negative values expand. Never invert dimensions.
    void Inset(int inset) {
        const float dx=std::min(static_cast<float>(inset),width/2);
        const float dy=std::min(static_cast<float>(inset),height/2);
        topLeft+=Point2D(dx,dy); width-=2*dx; height-=2*dy;
    }
    // Boundary points and touching rectangles count as contained/overlapping.
    bool IsInside(const Point2D &p) const {
        return p.x>=topLeft.x && p.x<=topLeft.x+width && p.y>=topLeft.y && p.y<=topLeft.y+height;
    }
    bool Overlaps(const Rect &b) const {
        return topLeft.x<=b.topLeft.x+b.width && topLeft.x+width>=b.topLeft.x &&
               topLeft.y<=b.topLeft.y+b.height && topLeft.y+height>=b.topLeft.y;
    }
};
inline std::ostream &operator<<(std::ostream &os,const Rect &r) { return os<<r.topLeft<<" ["<<r.width<<" x "<<r.height<<']'; }
// Rotate all four corners and return their enclosing screen-space AABB.
inline Rect WorldBounds(Rect r, Point2D location, float rotation) {
    Rect result(ToWorld(r.topLeft, location, rotation));
    result |= ToWorld(r.topLeft + Point2D(r.width, 0), location, rotation);
    result |= ToWorld(r.topLeft + Point2D(0, r.height), location, rotation);
    result |= ToWorld(r.topLeft + Point2D(r.width, r.height), location, rotation);
    return result;
}
} // namespace CMPUT350
#endif
