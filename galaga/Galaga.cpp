#include "Galaga.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Player.h"
#include "Stars.h"
#include "GameContext.h"
#include "Keyboard.h"
#include "NotificationManager.h"
using namespace CMPUT350;

Galaga::Galaga(int width,int height) : mWidth(width),mHeight(height) {}
void Galaga::Initialize(GameContext *context) {
    auto *engine=context->mEngineView;
    for (auto key : {Key::A,Key::Left}) {
        engine->InstallKeyDownNotification(static_cast<int>(key),"MoveLeftDown");
        engine->InstallKeyUpNotification(static_cast<int>(key),"MoveLeftUp");
    }
    for (auto key : {Key::D,Key::Right}) {
        engine->InstallKeyDownNotification(static_cast<int>(key),"MoveRightDown");
        engine->InstallKeyUpNotification(static_cast<int>(key),"MoveRightUp");
    }
    engine->InstallKeyDownNotification(static_cast<int>(Key::Space),"FireDown");
    engine->InstallKeyUpNotification(static_cast<int>(Key::Space),"FireUp");
    engine->InstallKeyDownNotification(static_cast<int>(Key::Num5),"InsertCoin");
    engine->InstallKeyDownNotification(static_cast<int>(Key::Num1),"StartGame");
    for (const auto *message : {"InsertCoin","StartGame","PlayerDestroyed","EnemyDestroyed:100",
                               "EnemyDestroyed:200","EnemyDestroyed:300","EnemyDestroyed:400","EnemyDestroyed:600"})
        context->mNotificationManager->Register(context->CurrObject,message);
    engine->AddGameObject(std::make_shared<Stars>(250,Rect(0,0,static_cast<float>(mWidth),static_cast<float>(mHeight))));
}
void Galaga::ReceiveNotification(const std::string &message) {
    if (message=="InsertCoin") ++mCredits;
    else if (message=="StartGame" && mState==State::Waiting && mCredits>0) mStartRequested=true;
    else if (message=="PlayerDestroyed" && mState!=State::Waiting && mState!=State::GameOver && !mPlayerLost) {
        mPlayerLost=true;
        --mLives;
        mRespawnAt=-1;
    } else if (message.rfind("EnemyDestroyed:",0)==0 && mState!=State::Waiting && mState!=State::GameOver) {
        mScore+=std::stoi(message.substr(15));
        mRemaining=std::max(0,mRemaining-1);
    }
}
void Galaga::Enter(State state,int frame) { mState=state; mStateFrame=frame; }
void Galaga::ClearRound(GameContext *context) {
    // Reset quietly: cleanup must not award points or consume a player life.
    for (const auto &object : *context->mEngineView)
        if (std::dynamic_pointer_cast<CollisionObject>(object)) object->GameObject::Kill();
    mPlayer.reset();
}
void Galaga::SpawnPlayer(GameContext *context) {
    auto player=std::make_shared<Player>(Point2D(mWidth/2.f,mHeight-100.f));
    mPlayer=player;
    mPlayerLost=false;
    mRespawnAt=-1;
    mNextAttack=context->currentFrame+90;
    context->mEngineView->AddGameObject(player);
}
bool Galaga::AllInFormation(GameContext *context) const {
    int count=0;
    for (const auto &object : *context->mEngineView) {
        if (auto enemy=std::dynamic_pointer_cast<Enemy>(object); enemy && enemy->IsAlive()) {
            ++count;
            if (!enemy->IsInFormation()) return false;
        }
    }
    return count==mRemaining; // Include ships still waiting in the engine queue.
}
void Galaga::ManageRespawn(GameContext *context) {
    if (!mPlayerLost || !AllInFormation(context)) { mRespawnAt=-1; return; }
    if (mRespawnAt<0) mRespawnAt=context->currentFrame+60;
    if (context->currentFrame>=mRespawnAt) SpawnPlayer(context);
}
void Galaga::SpawnEnemy(GameContext *context) {
    const int row=mSpawned/8,column=mSpawned%8;
    const Point2D home(mWidth*(column+1)/9.f,110.f+row*60);
    const float side=row%2==0 ? 1.f : -1.f;
    const Point2D start(side>0 ? -50.f : mWidth+50.f,60.f);
    const std::vector<Point2D> path={start,{mWidth/2.f,20},{mWidth/2.f+side*230,450},
                                   {mWidth/2.f,480},{mWidth/2.f-side*220,500},home+Vector2D(0,160),home};
    std::shared_ptr<Enemy> enemy;
    if ((row+column)%3==0) enemy=std::make_shared<Enemy1>(home,path);
    else if ((row+column)%3==1) enemy=std::make_shared<Enemy2>(home,path);
    else enemy=std::make_shared<Enemy3>(home,path);
    context->mEngineView->AddGameObject(enemy);
    ++mSpawned; ++mRemaining;
    // Eight ships per wave, with a small gap between the five waves.
    mNextSpawn=context->currentFrame+(mSpawned%8==0 ? 30 : 7);
}
void Galaga::Update(GameContext *context) {
    const int frame=context->currentFrame;
    if (mState==State::Waiting) {
        if (mStartRequested) {
            mStartRequested=false; --mCredits;
            ClearRound(context);
            mScore=0; mLives=3; mLevel=1; mSpawned=mRemaining=0; mAttackCursor=0;
            SpawnPlayer(context);
            Enter(State::Startup,frame);
        }
        return;
    }
    if (mState==State::GameOver) {
        if (frame-mStateFrame>=150) {
            ClearRound(context);
            mPlayerLost=false;
            Enter(State::Waiting,frame);
        }
        return;
    }
    if (mLives<=0) {
        context->mNotificationManager->Notify("ResetControls");
        ClearRound(context);
        Enter(State::GameOver,frame);
        return;
    }
    if (mState==State::Startup) {
        if (frame-mStateFrame>=90) {
            mSpawned=mRemaining=0;
            mNextSpawn=frame;
            Enter(State::Waves,frame);
        }
        return;
    }
    if (mState==State::Waves) {
        if (mSpawned<40 && frame>=mNextSpawn) SpawnEnemy(context);
        if (mSpawned==40 && AllInFormation(context)) {
            mNextAttack=frame+60;
            Enter(State::Playing,frame);
        }
    }
    if (mState==State::Waves || mState==State::Playing) ManageRespawn(context);
    if (mSpawned==40 && mRemaining==0) {
        ClearRound(context);
        ++mLevel;
        SpawnPlayer(context);
        Enter(State::Startup,frame);
        return;
    }
    if (mState==State::Playing && !mPlayerLost && frame>=mNextAttack) {
        auto player=mPlayer.lock();
        if (player && player->IsAlive()) {
            std::vector<std::shared_ptr<Enemy>> ready;
            for (const auto &object : *context->mEngineView)
                if (auto enemy=std::dynamic_pointer_cast<Enemy>(object); enemy && enemy->IsAlive() && enemy->IsInFormation())
                    ready.push_back(enemy);
            const int count=std::min(static_cast<int>(ready.size()),std::min(4,2+mLevel/3));
            for (int i=0;i<count;++i)
                ready[(mAttackCursor+i)%ready.size()]->Attack(context,player->GetLocation());
            mAttackCursor+=count;
        }
        mNextAttack=frame+std::max(40,100-mLevel*5);
    }
}
void Galaga::RenderUI(GameContext *context) {
    auto *screen=context->GUIContext;
    const float center=mWidth/2.f;
    screen->DrawText("CREDITS "+std::to_string(mCredits),18,{20,mHeight-30.f},Colors::yellow);
    if (mState==State::Waiting) {
        screen->DrawCenteredText("GALAGA",64,{center,260},Colors::cyan);
        screen->DrawCenteredText("5  INSERT COIN",26,{center,400},Colors::white);
        screen->DrawCenteredText(mCredits>0 ? "1  START GAME" : "INSERT A COIN TO PLAY",24,{center,450},Colors::yellow);
        screen->DrawCenteredText("A / D OR ARROWS  MOVE",20,{center,570},Colors::white);
        screen->DrawCenteredText("SPACE  FIRE",20,{center,610},Colors::white);
        screen->DrawCenteredText("THREE ENEMY TYPES - FIVE WAVES",16,{center,690},Colors::magenta);
        return;
    }
    screen->DrawText("SCORE "+std::to_string(mScore),22,{20,20},Colors::white);
    screen->DrawCenteredText("LIVES "+std::to_string(mLives),18,{mWidth-90.f,30},Colors::cyan);
    screen->DrawCenteredText("LEVEL "+std::to_string(mLevel),18,{center,30},Colors::yellow);
    if (mState==State::Startup) screen->DrawCenteredText("LEVEL "+std::to_string(mLevel),36,{center,500},Colors::yellow);
    if (mState==State::Waves)
        screen->DrawCenteredText("WAVE "+std::to_string(std::min(5,mSpawned/8+1))+" / 5",16,{center,65},Colors::gray);
    if (mPlayerLost && mLives>0)
        screen->DrawCenteredText("GET READY - "+std::to_string(mLives)+" LIVES",26,{center,720},Colors::cyan);
    if (mState==State::GameOver) {
        screen->DrawCenteredText("GAME OVER",48,{center,450},Colors::red);
        screen->DrawCenteredText("FINAL SCORE "+std::to_string(mScore),26,{center,520},Colors::white);
    }
}
Galaga::State Galaga::GetState() const { return mState; }
int Galaga::GetScore() const { return mScore; }
int Galaga::GetLives() const { return mLives; }
int Galaga::GetLevel() const { return mLevel; }
int Galaga::GetCredits() const { return mCredits; }
int Galaga::GetRemainingEnemies() const { return mRemaining; }
