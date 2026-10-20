#include "Enemy.h"
#include "Bullet.h"
#include "Player.h"
#include "GameContext.h"
#include "NotificationManager.h"
using namespace CMPUT350;
namespace { constexpr float pi=3.14159265358979323846f; }

Enemy::Enemy(Point2D home,const std::vector<Point2D> &path,int health,int points,int style)
    : mHome(home),mLocation(path.empty() ? home : path.front()),
      mShapes{Shape(Circle({0,-3},13)),Shape(Rect(-21,0,42,12))},
      mHealth(health),mPoints(points),mStyle(style) {
    SetPath(path,Motion::Entering);
}
void Enemy::Initialize(GameContext *context) { mNotifications=context->mNotificationManager; }
void Enemy::SetPath(const std::vector<Point2D> &points,Motion motion) {
    mPath=std::make_unique<Bezier>(points);
    mDistance=0;
    mMotion=motion;
}
void Enemy::Update(GameContext *context) {
    mFrame=context->currentFrame;
    if (mMotion==Motion::Formation) {
        mLocation=mHome+Vector2D(std::sin(mFrame*.035f)*16,0);
        mRotation=0;
        return;
    }
    mDistance=std::min(mDistance+mSpeed,mPath->GetLength());
    const float t=mPath->GetTimeAtDistance(mDistance);
    mLocation=mPath->GetPoint(t);
    auto tangent=mPath->GetSlope(t);
    if (tangent.Dot(tangent)>0.00001f) mRotation=std::atan2(tangent.y,tangent.x)+pi/2;
    if (mMotion==Motion::Attacking) {
        if (++mAttackFrames%30==12) Fire(context);
    }
    if (mDistance>=mPath->GetLength()) {
        if (mMotion==Motion::Attacking) {
            const auto start=mLocation;
            SetPath({start,start+Vector2D(100,-180),mHome+Vector2D(-100,120),mHome},Motion::Returning);
        } else {
            mMotion=Motion::Formation;
            mLocation=mHome;
            mRotation=0;
        }
    }
}
void Enemy::Fire(GameContext *context) {
    auto target=mTarget;
    for (const auto &object : *context->mEngineView)
        if (auto player=std::dynamic_pointer_cast<Player>(object); player && player->IsAlive())
            target=player->GetLocation();
    auto direction=target-mLocation;
    direction.Normalize();
    context->mEngineView->AddGameObject(std::make_shared<Bullet>(mLocation,direction*10.f,false,context->CurrObject));
}
bool Enemy::BeginAttack(GameContext *context,Point2D target,int style) {
    if (!IsAlive() || !IsInFormation()) return false;
    const auto start=mLocation;
    const float height=static_cast<float>(context->ScreenContext->GetWindowHeight());
    const float side=start.x<context->ScreenContext->GetWindowWidth()/2.f ? 1.f : -1.f;
    mTarget=target;
    mAttackFrames=0;
    mSpeed=6.f;
    if (style==0) {
        SetPath({start,start+Vector2D(110*side,100),target-Vector2D(80*side,180),{target.x,height+50}},Motion::Attacking);
    } else if (style==1) {
        const Point2D bend(target.x-150*side,target.y-180);
        SetPath({start,start+Vector2D(-170*side,130),bend+Vector2D(-120*side,-160),bend,
                 bend+Vector2D(170*side,180),{target.x+150*side,height-100},{target.x,height+60}},Motion::Attacking);
    } else {
        const Point2D bend(target.x+100*side,target.y-240);
        SetPath({start,start+Vector2D(230*side,80),bend+Vector2D(100*side,-100),bend,
                 bend+Vector2D(-260*side,140),{target.x-180*side,height-100},{target.x,height+50}},Motion::Attacking);
    }
    return true;
}
bool Enemy1::Attack(GameContext *c,const Point2D &target) { return BeginAttack(c,target,0); }
bool Enemy2::Attack(GameContext *c,const Point2D &target) { return BeginAttack(c,target,1); }
bool Enemy3::Attack(GameContext *c,const Point2D &target) { return BeginAttack(c,target,2); }
void Enemy::RenderForeground(GameContext *context) {
    auto *screen=context->ScreenContext;
    const RGBColor color=mHealth==1 && mStyle!=0 ? Colors::red :
                         mStyle==0 ? Colors::cyan : mStyle==1 ? Colors::green : Colors::magenta;
    if (mStyle==0) {
        screen->DrawRect({-20,0,40,10},color);
        screen->DrawRect({-6,-17,12,28},color);
    } else if (mStyle==1) {
        screen->DrawCircle({0,-3},14,color);
        screen->DrawRect({-21,0,42,10},color);
        screen->DrawCircle({0,-6},5,Colors::black);
    } else {
        screen->DrawLine({-20,10},{0,-16},8,color);
        screen->DrawLine({0,-16},{20,10},8,color);
        screen->DrawCircle({},7,Colors::yellow);
    }
}
void Enemy::CollisionEnter(const std::shared_ptr<CollisionObject> &object) {
    if (!GameObject::IsAlive()) return;
    if (auto bullet=std::dynamic_pointer_cast<Bullet>(object); bullet && bullet->IsPlayerBullet()) {
        if (--mHealth<=0) Kill();
    } else if (std::dynamic_pointer_cast<Player>(object)) Kill();
}
void Enemy::Kill() {
    if (!GameObject::IsAlive()) return;
    GameObject::Kill();
    if (mNotifications) mNotifications->Notify("EnemyDestroyed:"+std::to_string(mPoints*(IsInFormation() ? 1 : 2)));
}
bool Enemy::IsInFormation() const { return mMotion==Motion::Formation; }
int Enemy::GetHealth() const { return mHealth; }
const Rect &Enemy::GetBounds() { return mBounds; }
const std::vector<Shape> &Enemy::GetShapes() { return mShapes; }
Point2D Enemy::GetLocation() const { return mLocation; }
float Enemy::GetRotation() const { return mRotation; }
