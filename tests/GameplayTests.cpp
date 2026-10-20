#include "Galaga.h"
#include "Enemy.h"
#include "Player.h"
#include "Bullet.h"
#include "GameContext.h"
#include "NotificationManager.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace CMPUT350;
namespace {
void Check(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }

// A fast deterministic simulation host for the actual game objects. The real
// window/event loop is tested separately in EngineLoopTests.
class World : public EngineView {
public:
    std::vector<std::shared_ptr<GameObject>> active,pending;
    NotificationManager notifications;
    std::shared_ptr<sf::RenderWindow> window=std::make_shared<sf::RenderWindow>(
        sf::VideoMode({768,1024}),"Gameplay regression",sf::Style::Titlebar);
    DrawContext drawing{window,nullptr};
    GameContext context{this,&drawing};
    World() {
        window->setVisible(false);
        context.mNotificationManager=&notifications;
        context.GUIContext=&drawing;
    }
    void AddGameObject(std::shared_ptr<GameObject> object) override { pending.push_back(object); }
    void InstallKeyDownNotification(int,const std::string &) override {}
    void InstallKeyUpNotification(int,const std::string &) override {}
    const_iterator cbegin() const override { return active.cbegin(); }
    const_iterator cend() const override { return active.cend(); }
    const_iterator begin() const override { return cbegin(); }
    const_iterator end() const override { return cend(); }
    void Step() {
        std::erase_if(active,[](const auto &o) { return !o->IsAlive(); });
        auto added=std::move(pending);
        pending.clear();
        for (auto &object : added) {
            if (!object->IsAlive()) continue;
            active.push_back(object); context.CurrObject=object; object->Initialize(&context);
        }
        for (auto &object : active) {
            context.CurrObject=object;
            if (object->IsAlive()) object->Update(&context);
        }
        for (auto &object : active) {
            context.CurrObject=object;
            if (object->IsAlive()) object->LateUpdate(&context);
        }
        ++context.currentFrame;
    }
    std::shared_ptr<Player> PlayerObject() {
        for (auto &object : active)
            if (auto p=std::dynamic_pointer_cast<Player>(object); p && p->IsAlive()) return p;
        return {};
    }
    std::vector<std::shared_ptr<Enemy>> Enemies() {
        std::vector<std::shared_ptr<Enemy>> result;
        for (auto &object : active)
            if (auto enemy=std::dynamic_pointer_cast<Enemy>(object); enemy && enemy->IsAlive()) result.push_back(enemy);
        return result;
    }
    template<class Predicate>
    void Until(Predicate done,int maxFrames=3000) {
        for (int i=0;i<maxFrames && !done();++i) Step();
        Check(done(),"simulation transition timed out");
    }
};
}
int main() {
    try {
        World world;
        auto game=std::make_shared<Galaga>(768,1024);
        world.AddGameObject(game); world.Step();
        Check(game->GetState()==Galaga::State::Waiting,"initial coin state");
        world.notifications.Notify("StartGame"); world.Step();
        Check(game->GetState()==Galaga::State::Waiting,"cannot start without credit");
        world.notifications.Notify("InsertCoin"); world.notifications.Notify("StartGame"); world.Step();
        Check(game->GetCredits()==0 && game->GetLives()==3 && game->GetScore()==0 &&
              game->GetState()==Galaga::State::Startup,"credit consumed/startup");
        world.Step();
        auto player=world.PlayerObject();
        Check(player!=nullptr,"player created");
        // Two physical keys bound to one action remain active until both release.
        world.notifications.Notify("MoveLeftDown"); world.notifications.Notify("MoveLeftDown");
        const float start=player->GetLocation().x;
        world.Step(); world.notifications.Notify("MoveLeftUp"); world.Step();
        Check(player->GetLocation().x<start-7,"multiple-key held movement");
        world.notifications.Notify("MoveLeftUp"); const float stopped=player->GetLocation().x; world.Step();
        Check(player->GetLocation().x==stopped,"key release stops movement");
        world.notifications.Notify("MoveRightDown");
        for (int i=0;i<150;++i) world.Step();
        Check(player->GetLocation().x==748,"screen clamp");
        world.notifications.Notify("MoveRightUp"); world.notifications.Notify("FireDown");
        for (int i=0;i<200;++i) {
            world.Step();
            int bullets=0;
            for (const auto &object : world.active)
                if (auto b=std::dynamic_pointer_cast<Bullet>(object); b && b->IsAlive() && b->IsPlayerBullet()) ++bullets;
            for (const auto &object : world.pending)
                if (auto b=std::dynamic_pointer_cast<Bullet>(object); b && b->IsAlive() && b->IsPlayerBullet()) ++bullets;
            Check(bullets<=2,"two bullet limit includes pending objects");
        }
        world.notifications.Notify("FireUp");
        world.Until([&] { return game->GetState()==Galaga::State::Playing; });
        auto enemies=world.Enemies();
        Check(enemies.size()==40 && game->GetRemainingEnemies()==40,"five waves of eight");
        int one=0,two=0,three=0;
        for (const auto &enemy : enemies) {
            one+=std::dynamic_pointer_cast<Enemy1>(enemy)!=nullptr;
            two+=std::dynamic_pointer_cast<Enemy2>(enemy)!=nullptr;
            three+=std::dynamic_pointer_cast<Enemy3>(enemy)!=nullptr;
            Check(enemy->IsInFormation(),"wait for all arrivals");
        }
        Check(one>0 && two>0 && three>0,"three enemy types");
        const float before=enemies[0]->GetLocation().x;
        for (int i=0;i<10;++i) world.Step();
        Check(enemies[0]->GetLocation().x!=before,"formation hover");
        world.Until([&] {
            auto live=world.Enemies();
            return std::any_of(live.begin(),live.end(),[](const auto &e) { return !e->IsInFormation(); });
        });
        world.Until([&] {
            return std::any_of(world.active.begin(),world.active.end(),[](const auto &o) {
                auto b=std::dynamic_pointer_cast<Bullet>(o); return b && b->IsAlive() && !b->IsPlayerBullet();
            });
        });
        Check(game->GetLives()==3,"enemy attacks and firing");
        player->Kill(); player->Kill();
        Check(game->GetLives()==2,"one death consumes one life");
        player.reset();
        world.Step();
        world.Until([&] { return world.PlayerObject()!=nullptr; });
        for (const auto &enemy : world.Enemies()) Check(enemy->IsInFormation(),"respawn waits for attackers to return");
        // Destroy all enemies through their real Kill notifications and verify reset.
        for (auto &enemy : world.Enemies()) enemy->Kill();
        Check(game->GetRemainingEnemies()==0 && game->GetScore()>0,"score/death notifications");
        world.Step();
        Check(game->GetState()==Galaga::State::Startup && game->GetLevel()==2,"next level startup");
        world.Step();
        Check(world.Enemies().empty(),"old enemies cleaned up");
        world.Until([&] { return game->GetState()==Galaga::State::Playing; });
        for (int expected : {1,0}) {
            world.PlayerObject()->Kill();
            Check(game->GetLives()==expected,"life decrement");
            world.Step();
            if (expected>0) world.Until([&] { return world.PlayerObject()!=nullptr; });
        }
        Check(game->GetState()==Galaga::State::GameOver,"out of lives");
        Check(world.Enemies().empty(),"game over cleanup");
        world.Until([&] { return game->GetState()==Galaga::State::Waiting; });
        world.notifications.Notify("InsertCoin"); world.notifications.Notify("StartGame"); world.Step();
        Check(game->GetState()==Galaga::State::Startup && game->GetLevel()==1 &&
              game->GetScore()==0 && game->GetLives()==3,"fresh restart");
        std::cout<<"Gameplay tests passed\n";
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
