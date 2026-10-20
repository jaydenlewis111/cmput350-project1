#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "GameContext.h"
#include "NotificationManager.h"
#include <algorithm>
using namespace CMPUT350;

Player::Player(Point2D location) : mLocation(location),
    mShapes{Shape(Rect(-7,-20,14,36)),Shape(Rect(-20,1,40,14))} {}
void Player::Initialize(GameContext *context) {
    mNotifications = context->mNotificationManager;
    mInvulnerableUntil = context->currentFrame + 60;
    for (const auto *message : {"MoveLeftDown","MoveLeftUp","MoveRightDown","MoveRightUp","FireDown","FireUp","ResetControls"})
        mNotifications->Register(context->CurrObject,message);
}
void Player::ReceiveNotification(const std::string &message) {
    if (message=="MoveLeftDown") ++mLeft;
    else if (message=="MoveLeftUp") mLeft=std::max(0,mLeft-1);
    else if (message=="MoveRightDown") ++mRight;
    else if (message=="MoveRightUp") mRight=std::max(0,mRight-1);
    else if (message=="FireDown") ++mFire;
    else if (message=="FireUp") mFire=std::max(0,mFire-1);
    else if (message=="ResetControls") mLeft=mRight=mFire=0;
}
void Player::Fire(GameContext *context) {
    for (auto &slot : mBullets) {
        if (!slot.expired()) continue;
        auto bullet=std::make_shared<Bullet>(mLocation-Point2D(0,23),Vector2D(0,-24),true,context->CurrObject);
        slot=bullet;
        context->mEngineView->AddGameObject(bullet);
        mNextShot=context->currentFrame+7;
        break;
    }
}
void Player::Update(GameContext *context) {
    mFrame=context->currentFrame;
    mLocation.x += ((mRight>0 ? 1 : 0)-(mLeft>0 ? 1 : 0))*7.f;
    const float right=static_cast<float>(context->ScreenContext->GetWindowWidth())-20;
    mLocation.x=std::clamp(mLocation.x,20.f,std::max(20.f,right));
    if (mFire>0 && context->currentFrame>=mNextShot) Fire(context);
}
void Player::RenderForeground(GameContext *context) {
    if (mFrame<mInvulnerableUntil && (mFrame/4)%2==0) return;
    auto *screen=context->ScreenContext;
    screen->DrawRect({-6,-20,12,36},Colors::cyan);
    screen->DrawRect({-20,2,40,14},Colors::blue);
    screen->DrawRect({-4,-8,8,10},Colors::white);
    screen->DrawRect({-5,16,10,4},Colors::yellow);
}
void Player::CollisionEnter(const std::shared_ptr<CollisionObject> &object) {
    if (mFrame<mInvulnerableUntil) return;
    const auto bullet=std::dynamic_pointer_cast<Bullet>(object);
    if ((bullet && !bullet->IsPlayerBullet()) || std::dynamic_pointer_cast<Enemy>(object)) Kill();
}
void Player::Kill() {
    if (!GameObject::IsAlive()) return;
    GameObject::Kill();
    if (mNotifications) mNotifications->Notify("PlayerDestroyed");
}
const Rect &Player::GetBounds() { return mBounds; }
const std::vector<Shape> &Player::GetShapes() { return mShapes; }
Point2D Player::GetLocation() const { return mLocation; }
float Player::GetRotation() const { return 0; }
