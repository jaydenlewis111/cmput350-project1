#include "NotificationManager.h"
#include "GameObject.h"
#include <algorithm>
namespace CMPUT350 {
namespace {
// Weak ownership comparison works even after an object's lifetime ends.
bool SameOwner(const std::weak_ptr<GameObject> &a, const std::weak_ptr<GameObject> &b) {
    return !a.owner_before(b) && !b.owner_before(a);
}
bool Dead(const std::weak_ptr<GameObject> &object) {
    auto live = object.lock();
    return !live || !live->IsAlive();
}
}
void NotificationManager::Register(std::weak_ptr<GameObject> object, const std::string &key) {
    if (Dead(object)) return;
    auto &listeners = mListeners[key];
    std::erase_if(listeners, Dead);
    if (std::none_of(listeners.begin(), listeners.end(), [&](const auto &p) { return SameOwner(p, object); }))
        listeners.push_back(object);
}
void NotificationManager::Unregister(std::weak_ptr<GameObject> object, const std::string &key) {
    auto entry = mListeners.find(key);
    if (entry == mListeners.end()) return;
    std::erase_if(entry->second, [&](const auto &p) { return Dead(p) || SameOwner(p, object); });
    if (entry->second.empty()) mListeners.erase(entry);
}
void NotificationManager::Notify(const std::string &message) {
    auto entry = mListeners.find(message);
    if (entry == mListeners.end()) return;
    std::erase_if(entry->second, Dead);
    const auto snapshot = entry->second;
    if (entry->second.empty()) mListeners.erase(entry);
    for (const auto &listener : snapshot)
        if (auto object = listener.lock(); object && object->IsAlive()) object->ReceiveNotification(message);
}
}
