#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H
#include <string>

namespace CMPUT350 {

class GameContext;

class GameObject {
public:
    // Lifecycle hooks receive a borrowed context; default hooks do nothing.
    virtual ~GameObject() = default;
    // Called once on activation, before this object's first Update.
    virtual void Initialize(GameContext *context);
    // Called each live frame before collisions, with the current borrowed context.
    virtual void Update(GameContext *context);
    // Called after collision notifications for objects still alive.
    virtual void LateUpdate(GameContext *context);
    // Draw with GUIContext after all foregrounds; no object transform is applied.
    virtual void RenderUI(GameContext *context);
    // Receive a subscribed immediate message; the default ignores it.
    virtual void ReceiveNotification(const std::string &key);
    // IsAlive reports lifetime; Kill marks this object for next-frame removal.
    virtual bool IsAlive() const;
    virtual void Kill();
private:
    bool mAlive = true;
};

}  // namespace CMPUT350

#endif  // GAMEOBJECT_H
