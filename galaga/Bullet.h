#ifndef BULLET_H
#define BULLET_H
#include "CollisionObject.h"
class Bullet : public CMPUT350::CollisionObject {
public:
    Bullet(CMPUT350::Point2D location, CMPUT350::Vector2D heading, bool player,
           std::weak_ptr<CMPUT350::GameObject> parent);
    bool IsPlayerBullet() const;
    void Update(CMPUT350::GameContext *context) override;
    void LateUpdate(CMPUT350::GameContext *context) override;
    void RenderForeground(CMPUT350::GameContext *context) override;
    bool IsAlive() const override;
    void Kill() override;
    const std::vector<CMPUT350::Shape> &GetShapes() override;
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject> &obj) override;
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject> &obj,
                        const CMPUT350::Point2D &point) override;
    const CMPUT350::Rect &GetBounds() override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;
private:
    bool IsTarget(const std::shared_ptr<CMPUT350::CollisionObject> &obj) const;
    CMPUT350::Point2D mLocation;
    CMPUT350::Vector2D mHeading;
    CMPUT350::Point2D mImpact;
    CMPUT350::Rect mBounds;
    std::vector<CMPUT350::Shape> mShapes;
    std::weak_ptr<CMPUT350::GameObject> mParent;
    bool mPlayer, mHit = false;
    int mExplosionFrames = 0;
};
#endif
