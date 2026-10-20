#include "GameEngine.h"
#include "CollisionObject.h"
#include "DrawContext.h"
#include "GameContext.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace CMPUT350 {
#include "FontData.h"
namespace {
// Append a binding only once; multiple notifications per scancode are allowed.
void Install(std::unordered_map<int,std::vector<std::string>> &bindings, int key, const std::string &message) {
    auto &list=bindings[key];
    if (std::find(list.begin(),list.end(),message)==list.end()) list.push_back(message);
}
}
GameEngine::GameEngine(unsigned int width, unsigned int height, const std::string &name)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width,height}), name,
                                               sf::Style::Titlebar | sf::Style::Close)),
      mFont(std::make_shared<sf::Font>()) {
    mWindow->setFramerateLimit(30);
    mWindow->setKeyRepeatEnabled(false);
    if (!mFont->openFromMemory(_font,_font_len)) throw std::runtime_error("Unable to load embedded font");
}
GameEngine::~GameEngine() { mWindow->close(); }
void GameEngine::AddGameObject(std::shared_ptr<GameObject> object) {
    if (object) mPendingObjects.push_back(std::move(object));
}
void GameEngine::InstallKeyDownNotification(int key,const std::string &message) { Install(mKeyDown,key,message); }
void GameEngine::InstallKeyUpNotification(int key,const std::string &message) { Install(mKeyUp,key,message); }
EngineView::const_iterator GameEngine::cbegin() const { return mGameObjects.cbegin(); }
EngineView::const_iterator GameEngine::cend() const { return mGameObjects.cend(); }
EngineView::const_iterator GameEngine::begin() const { return cbegin(); }
EngineView::const_iterator GameEngine::end() const { return cend(); }
void GameEngine::SendKey(int key,bool down) {
    const auto &bindings=down ? mKeyDown : mKeyUp;
    auto found=bindings.find(key);
    if (found==bindings.end()) return;
    const auto messages=found->second;
    for (const auto &message : messages) mNotifications.Notify(message);
}
void GameEngine::Run() {
    DrawContext drawing(mWindow,mFont),gui(mWindow,mFont);
    GameContext context(this,&drawing);
    context.GUIContext=&gui;
    context.mNotificationManager=&mNotifications;
    while (mWindow->isOpen()) {
        std::erase_if(mGameObjects,[](const auto &o) { return !o->IsAlive(); });
        std::vector<std::shared_ptr<GameObject>> added;
        added.swap(mPendingObjects);
        for (auto &object : added) {
            if (!object->IsAlive()) continue;
            mGameObjects.push_back(object);
            context.CurrObject=object;
            object->Initialize(&context);
        }
        context.CurrObject.reset();
        while (const auto event=mWindow->pollEvent()) {
            if (event->is<sf::Event::Closed>()) mWindow->close();
            if (const auto *key=event->getIf<sf::Event::KeyPressed>()) {
                const int code=static_cast<int>(key->scancode);
                if (mPressedKeys.insert(code).second) SendKey(code,true);
            }
            if (const auto *key=event->getIf<sf::Event::KeyReleased>()) {
                const int code=static_cast<int>(key->scancode);
                if (mPressedKeys.erase(code)) SendKey(code,false);
            }
            if (event->is<sf::Event::FocusLost>()) {
                const auto pressed=mPressedKeys;
                mPressedKeys.clear();
                for (int key : pressed) SendKey(key,false);
            }
        }
        if (!mWindow->isOpen()) break;
        for (auto &object : mGameObjects) {
            context.CurrObject=object;
            if (object->IsAlive()) object->Update(&context);
        }
        // Capture all contacts before callbacks mutate shapes or lifetime. This
        // lets a swept bullet select the nearest of several simultaneous contacts.
        struct Pair {
            std::shared_ptr<CollisionObject> a,b;
            CollisionResult contact;
        };
        std::vector<std::shared_ptr<CollisionObject>> colliders;
        for (auto &object : mGameObjects)
            if (auto c=std::dynamic_pointer_cast<CollisionObject>(object); c && c->IsAlive()) colliders.push_back(c);
        std::vector<Pair> contacts;
        for (std::size_t a=0;a<colliders.size();++a)
            for (std::size_t b=a+1;b<colliders.size();++b) {
                auto result=FindCollision(*colliders[a],*colliders[b]);
                if (result.hit) contacts.push_back({colliders[a],colliders[b],result});
            }
        for (const auto &pair : contacts) {
            if (pair.contact.pointA) pair.a->CollisionEnter(pair.b,*pair.contact.pointA);
            else pair.a->CollisionEnter(pair.b);
            if (pair.contact.pointB) pair.b->CollisionEnter(pair.a,*pair.contact.pointB);
            else pair.b->CollisionEnter(pair.a);
        }
        for (auto &object : mGameObjects) {
            context.CurrObject=object;
            if (object->IsAlive()) object->LateUpdate(&context);
        }
        mWindow->clear(sf::Color::Black);
        // All backgrounds precede all foregrounds. Every object gets its own frame.
        for (int layer=0;layer<2;++layer) {
            for (auto &object : mGameObjects) {
                auto graphics=std::dynamic_pointer_cast<GraphicsObject>(object);
                if (!graphics || !graphics->IsAlive()) continue;
                context.CurrObject=object;
                drawing.SetContextOffset(graphics->GetLocation());
                drawing.SetContextRotation(graphics->GetRotation());
                if (layer==0) graphics->RenderBackground(&context);
                else graphics->RenderForeground(&context);
            }
        }
        drawing.SetContextOffset({});
        drawing.SetContextRotation(0);
        for (auto &object : mGameObjects) {
            context.CurrObject=object;
            if (object->IsAlive()) object->RenderUI(&context);
        }
        context.CurrObject.reset();
        mWindow->display();
        ++context.currentFrame;
    }
}
} // namespace CMPUT350
