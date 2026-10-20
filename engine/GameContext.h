#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H
#include "DrawContext.h"
#include "EngineView.h"
#include "GameObject.h"
#include <memory>
namespace CMPUT350 {
class NotificationManager;
class GameContext {
public:
    // Borrowed services are valid during callbacks. CurrObject never owns an object.
    EngineView *mEngineView = nullptr;
    DrawContext *ScreenContext = nullptr;
    DrawContext *GUIContext = nullptr;
    std::weak_ptr<GameObject> CurrObject;
    NotificationManager *mNotificationManager = nullptr;
    int currentFrame = 0;
    // Compatibility alias for the Part 1a interface.
    EngineView *EngineContext = nullptr;
    GameContext(EngineView *engine = nullptr, DrawContext *screen = nullptr)
        : mEngineView(engine), ScreenContext(screen), EngineContext(engine) {}
};
}
#endif
