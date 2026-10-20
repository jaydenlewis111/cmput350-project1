#include "Bullet.h"
#include "Enemy.h"
#include "Player.h"
#include "GameContext.h"
using namespace CMPUT350;

Bullet::Bullet(Point2D location, Vector2D heading, bool player, std::weak_ptr<GameObject> parent)
    : mLocation(location), mHeading(heading), mBounds(Point2D{}), mParent(parent), mPlayer(player) {
    mShapes.emplace_back(Line({}, {}));
}
bool Bullet::IsPlayerBullet() const { return mPlayer; }
bool Bullet::IsAlive() const {
    const auto parent = mParent.lock();
    return GameObject::IsAlive() && parent && parent->IsAlive();
}
void Bullet::Kill() { GameObject::Kill(); }
void Bullet::Update(GameContext *) {
    if (mHit) {
        if (++mExplosionFrames >= 8) Kill();
        return;
    }
    mLocation += mHeading;
    // Local origin is the new endpoint; p1 is the previous endpoint.
    const Line segment(mHeading * -1.f, {});
    mShapes = {Shape(segment)};
    mBounds = Rect(segment.p1, segment.p2);
}
void Bullet::LateUpdate(GameContext *context) {
    if (mHit) return;
    const auto bounds = WorldBounds(mBounds, mLocation, 0);
    if (bounds.topLeft.x + bounds.width < 0 || bounds.topLeft.y + bounds.height < 0 ||
        bounds.topLeft.x > context->ScreenContext->GetWindowWidth() ||
        bounds.topLeft.y > context->ScreenContext->GetWindowHeight()) Kill();
}
void Bullet::RenderForeground(GameContext *context) {
    const auto color = mPlayer ? Colors::yellow : Colors::magenta;
    if (mHit) context->ScreenContext->DrawCircle(mImpact, 6.f - mExplosionFrames * .5f, color);
    else context->ScreenContext->DrawLine(mHeading * -1.f, {}, 3, color);
}
bool Bullet::IsTarget(const std::shared_ptr<CollisionObject> &object) const {
    return (mPlayer && std::dynamic_pointer_cast<Enemy>(object)) ||
           (!mPlayer && std::dynamic_pointer_cast<Player>(object));
}
void Bullet::CollisionEnter(const std::shared_ptr<CollisionObject> &object) { CollisionEnter(object, {}); }
void Bullet::CollisionEnter(const std::shared_ptr<CollisionObject> &object, const Point2D &point) {
    if (!IsTarget(object)) return;
    // The engine snapshots contacts first, so all simultaneous points are seen.
    // Player bullets travel upward: the greatest local y is the first hit.
    if (!mHit || point.y > mImpact.y) mImpact = point;
    mHit = true;
    mShapes.clear();
    mBounds = Rect(mImpact, 6.f);
}
const std::vector<Shape> &Bullet::GetShapes() { return mShapes; }
const Rect &Bullet::GetBounds() { return mBounds; }
Point2D Bullet::GetLocation() const { return mLocation; }
float Bullet::GetRotation() const { return 0; }
