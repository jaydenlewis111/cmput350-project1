#ifndef PLAYER_H
#define PLAYER_H
#include "CollisionObject.h"
#include <array>
class Bullet;
namespace CMPUT350 { class NotificationManager; }
class Player : public CMPUT350::CollisionObject {
public:
    explicit Player(CMPUT350::Point2D location);
    void Initialize(CMPUT350::GameContext *context) override;
    void Update(CMPUT350::GameContext *context) override;
    void ReceiveNotification(const std::string &message) override;
    void RenderForeground(CMPUT350::GameContext *context) override;
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject> &obj) override;
    const CMPUT350::Rect &GetBounds() override;
    const std::vector<CMPUT350::Shape> &GetShapes() override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;
    void Kill() override;
private:
    void Fire(CMPUT350::GameContext *context);
    CMPUT350::Point2D mLocation;
    CMPUT350::Rect mBounds{-20,-20,40,40};
    std::vector<CMPUT350::Shape> mShapes;
    std::array<std::weak_ptr<Bullet>,2> mBullets;
    CMPUT350::NotificationManager *mNotifications = nullptr;
    int mLeft = 0, mRight = 0, mFire = 0, mNextShot = 0, mFrame = 0, mInvulnerableUntil = 0;
};
#endif
