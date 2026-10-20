#include "DrawContext.h"
#include <SFML/Graphics.hpp>
#include <utility>

namespace CMPUT350 {
namespace {
sf::Color Color(RGBColor c) { return {c.r, c.g, c.b}; }
}
// Retain shared ownership of the window and font for this drawing context.
DrawContext::DrawContext(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font)
    : mWindow(std::move(window)), mFont(std::move(font)) {}

// Draw text with its visible upper-left corner at p, in pixels.
void DrawContext::DrawText(const std::string &text, int pixelSize, Point2D p, RGBColor c) {
    if (pixelSize <= 0) return;
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    label.setOrigin(label.getLocalBounds().position);
    const auto position = Transform(p);
    label.setPosition({position.x, position.y});
    label.setRotation(sf::radians(mRotation));
    label.setFillColor(Color(c));
    mWindow->draw(label);
}
// Draw text with its visible bounding box centered on p.
void DrawContext::DrawCenteredText(const std::string &text, int pixelSize, Point2D p, RGBColor c) {
    if (pixelSize <= 0) return;
    sf::Text label(*mFont, text, static_cast<unsigned int>(pixelSize));
    const auto bounds = label.getLocalBounds();
    label.setOrigin(bounds.position + bounds.size / 2.f);
    const auto position = Transform(p);
    label.setPosition({position.x, position.y});
    label.setRotation(sf::radians(mRotation));
    label.setFillColor(Color(c));
    mWindow->draw(label);
}
// Draw a filled circle centered at p. Nonpositive radii draw nothing.
void DrawContext::DrawCircle(Point2D p, float radius, RGBColor c) {
    if (radius <= 0) return;
    sf::CircleShape shape(radius);
    shape.setOrigin({radius, radius});
    const auto position = Transform(p);
    shape.setPosition({position.x, position.y});
    shape.setFillColor(Color(c));
    mWindow->draw(shape);
}
// Draw a filled pixel rectangle; empty rectangles draw nothing.
void DrawContext::DrawRect(Rect r, RGBColor c) {
    if (r.width <= 0 || r.height <= 0) return;
    sf::RectangleShape shape({r.width, r.height});
    const auto position = Transform(r.topLeft);
    shape.setPosition({position.x, position.y});
    shape.setRotation(sf::radians(mRotation));
    shape.setFillColor(Color(c));
    mWindow->draw(shape);
}
// Draw an inward outline with the supplied pixel thickness.
void DrawContext::FrameRect(Rect r, float width, RGBColor c) {
    if (width <= 0 || r.width <= 0 || r.height <= 0) return;
    sf::RectangleShape shape({r.width, r.height});
    const auto position = Transform(r.topLeft);
    shape.setPosition({position.x, position.y});
    shape.setRotation(sf::radians(mRotation));
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(Color(c));
    shape.setOutlineThickness(-std::min(width, std::min(r.width, r.height) / 2.f));
    mWindow->draw(shape);
}
// Build a quad perpendicular to the segment, centered on its endpoints.
void DrawContext::DrawLine(Point2D from, Point2D to, float width, RGBColor c) {
    if (width <= 0) return;
    auto direction = to - from;
    if (direction == Point2D{}) { DrawCircle(from, width / 2, c); return; }
    direction.Normalize();
    const Point2D offset(-direction.y * width / 2, direction.x * width / 2);
    sf::ConvexShape shape(4);
    const Point2D points[] = {from + offset, to + offset, to - offset, from - offset};
    for (std::size_t i = 0; i < 4; ++i) {
        const auto p = Transform(points[i]);
        shape.setPoint(i, {p.x, p.y});
    }
    shape.setFillColor(Color(c));
    mWindow->draw(shape);
}
// Return the window dimensions in pixels.
int DrawContext::GetWindowWidth() { return static_cast<int>(mWindow->getSize().x); }
int DrawContext::GetWindowHeight() { return static_cast<int>(mWindow->getSize().y); }
// Set the borrowed object's frame before invoking its draw callbacks.
void DrawContext::SetContextOffset(Point2D p) { mOffset = p; }
void DrawContext::SetContextRotation(float rotation) { mRotation = rotation; }
// Forward/inverse point transforms and the conservative rotated rectangle AABB.
Point2D DrawContext::Transform(Point2D p) { return ToWorld(p, mOffset, mRotation); }
Point2D DrawContext::ReverseTransform(Point2D p) { return ToLocal(p, mOffset, mRotation); }
Rect DrawContext::Transform(Rect r) { return WorldBounds(r, mOffset, mRotation); }
} // namespace CMPUT350
