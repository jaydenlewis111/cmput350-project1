#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace CMPUT350 {
class GameObject;
class NotificationManager {
public:
    // Subscribe once per object/key without retaining ownership; ignore dead objects.
    void Register(std::weak_ptr<GameObject> object, const std::string &key);
    // Remove this object's subscription to key, including expired weak references.
    void Unregister(std::weak_ptr<GameObject> object, const std::string &key);
    // Deliver immediately to a snapshot; new registrations apply to later sends.
    // Expired and killed listeners are pruned. Reentrant notification is supported.
    void Notify(const std::string &message);
private:
    std::unordered_map<std::string, std::vector<std::weak_ptr<GameObject>>> mListeners;
};
}
#endif
