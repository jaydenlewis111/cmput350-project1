
#ifndef GAMEENGINE_H
#define GAMEENGINE_H

namespace CMPUT350 {
class GameEngine;
}

#include "EngineView.h"
#include "GameObject.h"
#include "MathUtil.h"
#include "NotificationManager.h"
#include <unordered_map>
#include <unordered_set>
#include <SFML/Graphics.hpp>

namespace CMPUT350 {

class DrawContext;

class GameEngine : public EngineView {
public:
    // Create a fixed-size pixel window and embedded font; throws on load failure.
    GameEngine(unsigned int width, unsigned int height, const std::string& name);
    ~GameEngine(); // Close the window; shared resources release automatically.

    GameEngine(const GameEngine&) = delete;             // Prevent copy-construction
    GameEngine(GameEngine&&) = delete;                  // Prevent move-construction
    GameEngine& operator=(const GameEngine&) = delete;  // Prevent assignment
    GameEngine& operator=(GameEngine&&) = delete;       // Prevent move-assignment

    // Queue an object (null is ignored); initialization happens next frame.
    void AddGameObject(std::shared_ptr<GameObject> gameObject) override;
    // Install unique notification strings per physical press/release scancode.
    void InstallKeyDownNotification(int key, const std::string &notification) override;
    void InstallKeyUpNotification(int key, const std::string &notification) override;
    // Borrow const iterators over active objects; pending additions are excluded.
    const_iterator cbegin() const override;
    const_iterator cend() const override;
    const_iterator begin() const override;
    const_iterator end() const override;

    void Run(); // Process frames until a close event; returns no value.

private:
    // Deliver mapped messages from a copied list, allowing callbacks to add bindings.
    void SendKey(int key, bool down);
    NotificationManager mNotifications;
    std::unordered_map<int, std::vector<std::string>> mKeyDown, mKeyUp;
    std::unordered_set<int> mPressedKeys;
    std::shared_ptr<sf::RenderWindow> mWindow;
    std::shared_ptr<sf::Font> mFont;
    std::vector<std::shared_ptr<GameObject>> mGameObjects;
    std::vector<std::shared_ptr<GameObject>> mPendingObjects;
};

}  // namespace CMPUT350

#endif  // GAMEENGINE_H
