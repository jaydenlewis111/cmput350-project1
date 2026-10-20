#ifndef ENGINEVIEW_H
#define ENGINEVIEW_H
#include <memory>
#include <string>
#include <vector>
namespace CMPUT350 {
class GameObject;
class EngineView {
    using tGameObject = std::vector<std::shared_ptr<GameObject>>;
public:
    virtual ~EngineView() = default;
    // Queue a non-null object for initialization at the next frame boundary.
    virtual void AddGameObject(std::shared_ptr<GameObject> gameObject) = 0;
    // Associate a physical scancode with an immediate notification on press/release.
    virtual void InstallKeyDownNotification(int key, const std::string &notification) = 0;
    virtual void InstallKeyUpNotification(int key, const std::string &notification) = 0;
    using const_iterator = tGameObject::const_iterator;
    // Borrow active-object iterators. They are stable during callbacks, not across frames.
    virtual const_iterator cbegin() const = 0;
    virtual const_iterator cend() const = 0;
    virtual const_iterator begin() const = 0;
    virtual const_iterator end() const = 0;
};
}
#endif
