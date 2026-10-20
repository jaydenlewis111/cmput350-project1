#include "GameEngine.h"
#include "GameContext.h"
#include "CollisionObject.h"
#include "Keyboard.h"
#include "NotificationManager.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace CMPUT350;
namespace {
std::vector<std::string> events;
void Check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

// Window managers may reparent application windows below their own frame.
Window FindWindow(Display *display, Window root) {
    char *name = nullptr;
    if (XFetchName(display, root, &name) && name) {
        const bool found = std::string(name) == "CMPUT350 engine test";
        XFree(name);
        if (found) return root;
    }
    Window parent, returnedRoot, *children = nullptr;
    unsigned int count = 0;
    if (!XQueryTree(display, root, &returnedRoot, &parent, &children, &count)) return 0;
    Window found = 0;
    for (unsigned int i = 0; i < count && !found; ++i) found = FindWindow(display, children[i]);
    if (children) XFree(children);
    return found;
}

// Close only this test's native window through the normal event queue.
void RequestClose() {
    Display *display = XOpenDisplay(nullptr);
    Check(display != nullptr, "X display unavailable");
    const Window window = FindWindow(display, DefaultRootWindow(display));
    if (window) {
        XEvent event{};
        event.xclient.type = ClientMessage;
        event.xclient.window = window;
        event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
        event.xclient.format = 32;
        event.xclient.data.l[0] = static_cast<long>(XInternAtom(display, "WM_DELETE_WINDOW", False));
        XSendEvent(display, window, False, NoEventMask, &event);
    }
    XSync(display, False);
    XCloseDisplay(display);
    Check(window != 0, "test window not found");
}

// Inject physical press/press/release events into only this test window.
void SendKeyEvents() {
    Display *display=XOpenDisplay(nullptr);
    Check(display!=nullptr,"X display unavailable");
    const Window window=FindWindow(display,DefaultRootWindow(display));
    Check(window!=0,"key test window missing");
    XSetInputFocus(display,window,RevertToParent,CurrentTime);
    XSync(display,False);
    for (int type : {KeyPress,KeyPress,KeyRelease}) {
        XEvent event{};
        event.xkey.type=type;
        event.xkey.display=display;
        event.xkey.window=window;
        event.xkey.root=DefaultRootWindow(display);
        event.xkey.same_screen=True;
        event.xkey.keycode=XKeysymToKeycode(display,XK_a);
        XSendEvent(display,window,False,type==KeyPress ? KeyPressMask : KeyReleaseMask,&event);
    }
    XSync(display,False);
    XCloseDisplay(display);
}

struct Child : GameObject {
    void Initialize(GameContext *) override { events.push_back("child.initialize"); }
    void Update(GameContext *) override { events.push_back("child.update"); Kill(); }
    ~Child() override { events.push_back("child.destroy"); }
};
struct Collider : CollisionObject {
    explicit Collider(std::string name) : name(std::move(name)) {}
    std::string name;
    Rect bounds{10, 20, 30, 40};
    const Rect &GetBounds() override { return bounds; }
    const std::vector<Shape> &GetShapes() override { return shapes; }
    std::vector<Shape> shapes{Shape(Rect(10,20,30,40))};
    Point2D GetLocation() const override { return {}; }
    float GetRotation() const override { return 0; }
    void CollisionEnter(const std::shared_ptr<CollisionObject> &other) override {
        Check(other.get() != this, "self collision");
        events.push_back(name + ".collision");
        Kill();
    }
};
struct Controller : GraphicsObject {
    Point2D GetLocation() const override { return {10,15}; }
    float GetRotation() const override { return .25f; }
    int frame = 0;
    void Initialize(GameContext *c) override {
        events.push_back("initialize");
        Check(c->currentFrame==0,"initial frame zero");
        c->mEngineView->InstallKeyDownNotification(static_cast<int>(Key::A),"ProbeDown");
        c->mEngineView->InstallKeyDownNotification(static_cast<int>(Key::A),"ProbeDown");
        c->mEngineView->InstallKeyUpNotification(static_cast<int>(Key::A),"ProbeUp");
        c->mNotificationManager->Register(c->CurrObject,"ProbeDown");
        c->mNotificationManager->Register(c->CurrObject,"ProbeUp");
        c->EngineContext->AddGameObject(std::make_shared<Child>());
    }
    void ReceiveNotification(const std::string &message) override { events.push_back(message); }
    void Update(GameContext *c) override {
        ++frame;
        Check(c->currentFrame==frame-1,"global frame increments");
        Check(std::distance(c->mEngineView->begin(),c->mEngineView->end())>=1,"engine iterators");
        events.push_back("update" + std::to_string(frame));
        if (frame == 1) c->EngineContext->AddGameObject(std::make_shared<Child>());
    }
    void LateUpdate(GameContext *) override { events.push_back("late" + std::to_string(frame)); }
    void RenderBackground(GameContext *c) override {
        events.push_back("background" + std::to_string(frame));
        c->ScreenContext->DrawText("test", 12, {5, 5}, Colors::white);
        c->ScreenContext->DrawCenteredText("center", 12, {100, 100}, Colors::white);
        c->ScreenContext->DrawCircle({50, 50}, 10, Colors::cyan);
        c->ScreenContext->DrawRect({70, 70, 20, 20}, Colors::red);
        c->ScreenContext->FrameRect({90, 90, 20, 20}, 2, Colors::green);
        c->ScreenContext->DrawLine({20, 50}, {70, 80}, 4, Colors::yellow);
        c->ScreenContext->DrawLine({20, 50}, {20, 50}, 4, Colors::yellow);
    }
    void RenderForeground(GameContext *) override {
        events.push_back("foreground" + std::to_string(frame));
        if (frame == 8) RequestClose();
        if (frame == 3) SendKeyEvents();
    }
    void RenderUI(GameContext *context) override {
        Check(context->GUIContext != context->ScreenContext,"separate GUI context");
        Check(!context->CurrObject.expired(),"current object weak pointer");
        events.push_back("ui"+std::to_string(frame));
        context->GUIContext->DrawText("GUI",12,{5,5},Colors::white);
    }
};
std::size_t Index(const std::string &event) {
    auto i = std::find(events.begin(), events.end(), event);
    Check(i != events.end(), "missing callback");
    return static_cast<std::size_t>(i - events.begin());
}
}

int main() {
    try {
        {
            GameEngine engine(240, 180, "CMPUT350 engine test");
            auto first = std::make_shared<Collider>("a");
            auto second = std::make_shared<Collider>("b");
            std::weak_ptr<Collider> released = first;
            engine.AddGameObject(std::make_shared<Controller>());
            engine.AddGameObject(first); engine.AddGameObject(second);
            first.reset(); second.reset();
            engine.AddGameObject(nullptr);
            engine.Run();
            Check(released.expired(), "dead collider retained");
        }
        Check(Index("initialize") < Index("update1"), "initialization precedes update");
        Check(Index("update1") < Index("a.collision") && Index("b.collision") < Index("late1"), "collision order");
        Check(Index("late1") < Index("background1") && Index("background1") < Index("foreground1"), "render order");
        Check(Index("foreground1") < Index("ui1"),"UI follows foreground");
        Check(std::count(events.begin(),events.end(),"ProbeDown")==1 &&
              std::count(events.begin(),events.end(),"ProbeUp")==1,"scancode press/release, repeat and binding deduplication");
        Check(Index("foreground1") < Index("child.initialize"), "initialize/update additions must wait one frame");
        Check(Index("child.initialize") < Index("update2"), "new objects initialized before next update");
        Check(Index("child.destroy") < Index("update3"), "dead objects removed at next frame start");
        Check(std::count(events.begin(), events.end(), "a.collision") == 1 &&
              std::count(events.begin(), events.end(), "b.collision") == 1, "exactly one callback per pair");
        Check(std::count(events.begin(), events.end(), "child.initialize") == 2 &&
              std::count(events.begin(), events.end(), "child.update") == 2 &&
              std::count(events.begin(), events.end(), "child.destroy") == 2, "deferred children lifecycle");
        std::cout << "Engine loop tests passed\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
