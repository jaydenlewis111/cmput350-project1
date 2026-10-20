#ifndef ENEMY_H
#define ENEMY_H
#include "Bezier.h"
#include "CollisionObject.h"
namespace CMPUT350 { class NotificationManager; }

class Enemy : public CMPUT350::CollisionObject {
public:
    Enemy(CMPUT350::Point2D home, const std::vector<CMPUT350::Point2D> &path, int health, int points, int style);
    void Initialize(CMPUT350::GameContext *context) override;
    void Update(CMPUT350::GameContext *context) override;
    void RenderForeground(CMPUT350::GameContext *context) override;
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject> &obj) override;
    void Kill() override;
    const CMPUT350::Rect &GetBounds() override;
    const std::vector<CMPUT350::Shape> &GetShapes() override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;
    virtual bool Attack(CMPUT350::GameContext *context, const CMPUT350::Point2D &target) = 0;
    bool IsInFormation() const;
    int GetHealth() const;
protected:
    bool BeginAttack(CMPUT350::GameContext *context, CMPUT350::Point2D target, int style);
private:
    enum class Motion { Entering, Formation, Attacking, Returning };
    void SetPath(const std::vector<CMPUT350::Point2D> &points, Motion motion);
    void Fire(CMPUT350::GameContext *context);
    CMPUT350::Point2D mHome, mLocation, mTarget;
    std::unique_ptr<CMPUT350::Bezier> mPath;
    CMPUT350::Rect mBounds{-22,-18,44,36};
    std::vector<CMPUT350::Shape> mShapes;
    CMPUT350::NotificationManager *mNotifications = nullptr;
    Motion mMotion = Motion::Entering;
    float mDistance=0, mRotation=0, mSpeed=5;
    int mHealth, mPoints, mStyle, mFrame=0, mAttackFrames=0;
};
class Enemy1 : public Enemy {
public:
    Enemy1(CMPUT350::Point2D home,const std::vector<CMPUT350::Point2D> &path) : Enemy(home,path,1,100,0) {}
    bool Attack(CMPUT350::GameContext *context,const CMPUT350::Point2D &target) override;
};
class Enemy2 : public Enemy {
public:
    Enemy2(CMPUT350::Point2D home,const std::vector<CMPUT350::Point2D> &path) : Enemy(home,path,2,200,1) {}
    bool Attack(CMPUT350::GameContext *context,const CMPUT350::Point2D &target) override;
};
class Enemy3 : public Enemy {
public:
    Enemy3(CMPUT350::Point2D home,const std::vector<CMPUT350::Point2D> &path) : Enemy(home,path,2,300,2) {}
    bool Attack(CMPUT350::GameContext *context,const CMPUT350::Point2D &target) override;
};
#endif
