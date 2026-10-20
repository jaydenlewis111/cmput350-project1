#ifndef BEZIER_H
#define BEZIER_H
#include "MathUtil.h"
#include <vector>
namespace CMPUT350 {
class Bezier {
public:
    // Copy 3*k+1 control points (k >= 1); invalid lists throw invalid_argument.
    explicit Bezier(const std::vector<Point2D> &pts);
    // Evaluate position or derivative with t clamped to [0, number of segments].
    Point2D GetPoint(float t) const;
    Vector2D GetSlope(float t) const;
    static Point2D GetPoint(const std::vector<Point2D> &pts, float t);
    static Vector2D GetSlope(const std::vector<Point2D> &pts, float t);
    // Approximate arc length by 256 samples per segment; distance is in pixels.
    float GetLength() const;
    float GetTimeAtDistance(float distance) const;
private:
    std::vector<Point2D> mPoints;
    std::vector<float> mLengths;
};
}
#endif
