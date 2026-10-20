#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <cstdint>

#include "MathUtil.h"
#include <memory>
#include <string>

namespace sf { class RenderWindow; class Font; }

namespace CMPUT350 {

struct RGBColor {
    uint8_t r, g, b;
    RGBColor(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
};

namespace Colors {

const RGBColor red(255, 0, 0);
const RGBColor green(0, 255, 0);
const RGBColor blue(0, 0, 255);
const RGBColor yellow(255, 255, 0);
const RGBColor cyan(0, 255, 255);
const RGBColor magenta(255, 0, 255);
const RGBColor white(255, 255, 255);
const RGBColor black(0, 0, 0);
const RGBColor gray(100, 100, 100);
const RGBColor grey(200, 200, 200);

}  // namespace Colors

class DrawContext {
public:
    // Retain the supplied shared window and font for subsequent drawing calls.
    DrawContext(std::shared_ptr<sf::RenderWindow> mWindow, std::shared_ptr<sf::Font> font);
    // Draw text at its visible top-left (or center), using pixel font size.
    void DrawText(const std::string &text, int pixelSize, Point2D p, RGBColor c);
    void DrawCenteredText(const std::string &text, int pixelSize, Point2D p, RGBColor c);
    void DrawCircle(Point2D p, float radius, RGBColor c); // Filled circle centered on p.
    void DrawRect(Rect r, RGBColor c); // Filled rectangle at r.topLeft.
    void FrameRect(Rect r, float width, RGBColor c); // Inward outline in pixels.
    void DrawLine(Point2D from, Point2D to, float width, RGBColor c); // Quad centered on segment.
    // Return current pixel dimensions. Drawing methods have no return value.
    int GetWindowWidth();
    int GetWindowHeight();

private:
    friend class GameEngine;
    // Engine-only object frame setup. GUI contexts keep the identity transform.
    void SetContextOffset(Point2D p);
    void SetContextRotation(float rotation);
    Point2D Transform(Point2D p);
    Point2D ReverseTransform(Point2D p);
    Rect Transform(Rect r);
    Point2D mOffset;
    float mRotation = 0;
    std::shared_ptr<sf::RenderWindow> mWindow;
    std::shared_ptr<sf::Font> mFont;
};

}  // namespace CMPUT350

#endif  // RENDERTARGET_H
