#include "Bezier.h"
namespace CMPUT350 {
namespace {
// Validate the piecewise cubic layout, then select a segment and its local t.
std::size_t Segment(const std::vector<Point2D> &pts, float &t) {
    if (pts.size() < 4 || (pts.size() - 1) % 3 != 0 || !std::isfinite(t))
        throw std::invalid_argument("Bezier requires 3*k+1 points and finite time");
    const auto count = (pts.size() - 1) / 3;
    t = std::clamp(t, 0.f, static_cast<float>(count));
    const auto segment = std::min(static_cast<std::size_t>(t), count - 1);
    t -= static_cast<float>(segment);
    return segment * 3;
}
}
Bezier::Bezier(const std::vector<Point2D> &pts) : mPoints(pts) {
    float t = 0;
    Segment(pts, t);
    const std::size_t samples = ((pts.size() - 1) / 3) * 256;
    mLengths.reserve(samples + 1);
    mLengths.push_back(0);
    auto previous = GetPoint(0);
    for (std::size_t i = 1; i <= samples; ++i) {
        auto point = GetPoint(static_cast<float>(i) / 256);
        mLengths.push_back(mLengths.back() + point.Distance(previous));
        previous = point;
    }
}
Point2D Bezier::GetPoint(float t) const { return GetPoint(mPoints, t); }
Vector2D Bezier::GetSlope(float t) const { return GetSlope(mPoints, t); }
Point2D Bezier::GetPoint(const std::vector<Point2D> &pts, float t) {
    const auto i = Segment(pts, t);
    const float u = 1 - t;
    return pts[i] * (u*u*u) + pts[i+1] * (3*u*u*t) + pts[i+2] * (3*u*t*t) + pts[i+3] * (t*t*t);
}
Vector2D Bezier::GetSlope(const std::vector<Point2D> &pts, float t) {
    const auto i = Segment(pts, t);
    const float u = 1 - t;
    return (pts[i+1]-pts[i]) * (3*u*u) + (pts[i+2]-pts[i+1]) * (6*u*t) + (pts[i+3]-pts[i+2]) * (3*t*t);
}
float Bezier::GetLength() const { return mLengths.back(); }
float Bezier::GetTimeAtDistance(float distance) const {
    if (!std::isfinite(distance)) throw std::invalid_argument("Bezier distance must be finite");
    distance = std::clamp(distance, 0.f, GetLength());
    auto upper = std::lower_bound(mLengths.begin(), mLengths.end(), distance);
    if (upper == mLengths.begin()) return 0;
    const auto i = static_cast<std::size_t>(upper - mLengths.begin());
    const float span = mLengths[i] - mLengths[i-1];
    const float fraction = span > 0 ? (distance - mLengths[i-1]) / span : 0;
    return (static_cast<float>(i - 1) + fraction) / 256;
}
}
