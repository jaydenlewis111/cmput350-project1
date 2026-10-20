#include "MathUtil.h"
#include "Bezier.h"
#include "CollisionObject.h"
#include "NotificationManager.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Player.h"
#include "GameContext.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace CMPUT350;
namespace {
void Check(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
bool Near(float a,float b) { return std::abs(a-b)<.002f; }
bool Near(Point2D a,Point2D b) { return a.Distance(b)<.002f; }
class Body : public CollisionObject {
public:
    Body(std::vector<Shape> shapes,Rect bounds,Point2D location={},float rotation=0)
        : shapes(std::move(shapes)),bounds(bounds),location(location),rotation(rotation) {}
    std::vector<Shape> shapes;
    Rect bounds;
    Point2D location;
    float rotation;
    void CollisionEnter(const std::shared_ptr<CollisionObject> &) override {}
    const Rect &GetBounds() override { return bounds; }
    const std::vector<Shape> &GetShapes() override { return shapes; }
    Point2D GetLocation() const override { return location; }
    float GetRotation() const override { return rotation; }
};
class Listener : public GameObject {
public:
    int count=0;
    NotificationManager *manager=nullptr;
    bool reenter=false;
    void ReceiveNotification(const std::string &message) override {
        ++count;
        if (reenter) { reenter=false; manager->Unregister(self,message); manager->Notify(message); }
    }
    std::weak_ptr<GameObject> self;
};
void MathTests() {
    Point2D p(3,4);
    Check(Near(p.Distance({}),5),"distance");
    Check(p+Point2D(2,1)==Point2D(5,5),"point addition");
    Check(p-2.f==Point2D(1,2) && p+2.f==Point2D(5,6),"scalar arithmetic");
    Check(2.f*p==Point2D(6,8),"scalar multiplication");
    Check(p*Point2D(2,3)==18 && p.Cross({2,3})==1,"dot/cross");
    p.Normalize(); Check(Near(p,{.6f,.8f}),"normalization");
    Point2D zero; zero.Normalize(); Check(zero==Point2D{},"zero normalization");
    p={3,4}; p/=2.5f; Check(Near(p.x,1.2f),"float division");
    p={3,4}; p+=1.f; p+=Point2D(1,2); p-=Point2D(2,1); p*=2; p/=2;
    Check(p==Point2D(3,6),"compound arithmetic");
    bool threw=false; try { p/=0; } catch (const std::invalid_argument &) { threw=true; }
    Check(threw,"zero division");
    const Point2D local(13,-8),offset(120,200);
    for (float angle : {0.f,.7f,1.5707963f,-2.f})
        Check(Near(ToLocal(ToWorld(local,offset,angle),offset,angle),local),"inverse rotation");
    Rect bounds(10,20,30,40);
    Check(bounds.topLeft==Point2D(10,20),"rectangle coordinate order");
    Check(bounds.IsInside({40,60}) && !bounds.IsInside({41,60}),"containment");
    Check(bounds.Overlaps(Rect(40,60,5,5)) && !bounds.Overlaps(Rect(41,60,5,5)),"overlap boundaries");
    Check(Rect(Point2D(25,100),Point2D(25,0)).Overlaps(bounds),"degenerate bounds");
    Rect circleBounds(Point2D(10,20),5.f);
    Check(circleBounds.topLeft==Point2D(5,15) && circleBounds.width==10,"circle bounds");
    auto rotated=WorldBounds(Rect(-20,-5,40,10),{100,100},1.5707963f);
    Check(Near(rotated.width,10) && Near(rotated.height,40) && Near(rotated.topLeft,{95,80}),"rotated bounds");
    bounds|=Point2D(0,10); Check(bounds.width==40 && bounds.height==50,"union");
    bounds&=Rect(5,15,10,20); Check(bounds.topLeft==Point2D(5,15) && bounds.width==10,"intersection");
    bounds.Inset(100); Check(bounds.width==0 && bounds.height==0,"large inset");
    Point2D hit;
    Check(Line({0,0},{10,10}).Crosses(Line({0,10},{10,0}),hit) && Near(hit,{5,5}),"line intersection");
    Check(Line({10,0},{0,0}).Crosses(Line({4,0},{8,0}),hit) && Near(hit,{8,0}),"directed collinear contact");
    Check(!Line({0,0},{10,0}).Crosses(Line({0,1},{10,1}),hit),"parallel lines");
    Check(Line({0,0},{10,0}).ClosestPoint({20,5})==Point2D(10,0),"segment clamp");
    std::ostringstream out; out<<p<<bounds<<Line{}<<Circle{}; Check(!out.str().empty(),"stream helpers");
}
void CollisionTests() {
    Body circle({Circle({},10)},Rect(Point2D{},10.f),{100,100});
    Body other({Circle({},10)},Rect(Point2D{},10.f),{119,100});
    Check(FindCollision(circle,other).hit,"circle/circle");
    other.location.x=121; Check(!FindCollision(circle,other).hit,"circle separation");
    Body box({Rect(-20,-5,40,10)},Rect(-20,-5,40,10),{100,100},1.5707963f);
    Check(FindCollision(circle,box).hit,"rotated circle/rectangle");
    Body otherBox({Rect(-5,-5,10,10)},Rect(-5,-5,10,10),{100,119},.4f);
    Check(FindCollision(box,otherBox).hit,"rotated rectangles");
    otherBox.location={120,120}; Check(!FindCollision(box,otherBox).hit,"separated rotated rectangles");
    // Broad boxes overlap, while precise circles miss.
    Body a({Circle({},10)},Rect(Point2D{},10.f),{0,0});
    Body b({Circle({},10)},Rect(Point2D{},10.f),{18,18});
    Check(!FindCollision(a,b).hit,"broad phase is not a precise collision");
    Body line({Line({0,30},{0,-30})},Rect(0,-30,0,60),{100,100});
    auto result=FindCollision(line,circle);
    Check(result.hit && result.pointA && result.pointB && Near(*result.pointA,{0,10}) && Near(*result.pointB,{0,10}),"first line/circle point");
    result=FindCollision(line,box);
    Check(result.hit && result.pointB && Near(ToWorld(*result.pointB,box.location,box.rotation),{100,120}),"line/rotated rectangle local point");
    Body inside({Line({0,0},{0,-30})},Rect(0,-30,0,30),{100,100});
    result=FindCollision(inside,circle); Check(result.hit && Near(*result.pointA,{}),"inside line start");
    Body tangent({Line({-30,10},{30,10})},Rect(-30,10,60,0),{100,100});
    result=FindCollision(tangent,circle); Check(result.hit && Near(*result.pointA,{0,10}),"tangent line");
    Body point({Line({0,0},{0,0})},Rect(Point2D{}),{100,100});
    Check(FindCollision(point,circle).hit,"zero-length line inside circle");
    Body crossing({Line({-20,0},{20,0})},Rect(-20,0,40,0),{100,100});
    Check(FindCollision(line,crossing).hit,"line/line");
    Body compound({Circle({0,-15},3),Rect(-4,5,8,10)},Rect(-4,-18,8,33),{100,100});
    result=FindCollision(line,compound);
    Check(result.hit && Near(*result.pointA,{0,15}),"nearest directed component contact");
    Check(FindCollision(compound,line).hit,"symmetric shape ordering");
}
void BezierTests() {
    std::vector<Point2D> straight={{0,0},{10,0},{20,0},{30,0},{40,0},{50,0},{60,0}};
    Bezier curve(straight);
    Check(Near(curve.GetPoint(0),{0,0}) && Near(curve.GetPoint(1),{30,0}) && Near(curve.GetPoint(2),{60,0}),"segment endpoints");
    Check(Near(curve.GetPoint(.5f),{15,0}) && Near(curve.GetSlope(1.5f),{30,0}),"point/slope");
    Check(Near(Bezier::GetPoint(straight,2),curve.GetPoint(2)),"static equivalence");
    Check(Near(curve.GetLength(),60) && Near(curve.GetTimeAtDistance(45),1.5f),"arc length");
    Bezier bend({{0,0},{0,300},{30,-200},{100,0}});
    float previous=0;
    for (float distance=2;distance<bend.GetLength();distance+=2) {
        float t=bend.GetTimeAtDistance(distance);
        float arc=0;
        auto point=bend.GetPoint(previous);
        for (int sample=1;sample<=30;++sample) {
            auto next=bend.GetPoint(previous+(t-previous)*sample/30.f);
            arc+=point.Distance(next); point=next;
        }
        Check(t>=previous && std::abs(arc-2)<.06f,"constant-distance arc sampling");
        previous=t;
    }
    Check(Near(curve.GetPoint(-1),{0,0}) && Near(curve.GetPoint(10),{60,0}),"clamped evaluation");
    bool threw=false; try { Bezier invalid({{0,0},{1,1}}); } catch (const std::invalid_argument &) { threw=true; }
    Check(threw,"invalid bezier");
}
void NotificationTests() {
    NotificationManager manager;
    auto first=std::make_shared<Listener>(),second=std::make_shared<Listener>();
    manager.Register(first,"event"); manager.Register(first,"event"); manager.Register(second,"event");
    manager.Notify("unknown"); manager.Notify("event");
    Check(first->count==1 && second->count==1,"unique subscriptions");
    manager.Unregister(first,"event"); manager.Notify("event");
    Check(first->count==1 && second->count==2,"unregister");
    second->Kill(); manager.Notify("event"); Check(second->count==2,"killed listener pruned");
    std::weak_ptr<GameObject> weak=first; first.reset(); Check(weak.expired(),"manager does not own listeners");
    manager.Notify("event");
    auto temporary=std::make_shared<Listener>();
    manager.Register(temporary,"expired");
    weak=temporary; temporary.reset();
    Check(weak.expired(),"subscribed object is not retained");
    manager.Notify("expired");
    auto reentrant=std::make_shared<Listener>();
    reentrant->manager=&manager; reentrant->self=reentrant; reentrant->reenter=true;
    manager.Register(reentrant,"nested"); manager.Notify("nested");
    Check(reentrant->count==1,"reentrant unregister/notify");
}
void BulletEnemyTests() {
    auto parent=std::make_shared<GameObject>();
    auto enemy=std::make_shared<Enemy1>(Point2D(50,50),std::vector<Point2D>{{50,50},{50,50},{50,50},{50,50}});
    auto bullet=std::make_shared<Bullet>(Point2D(50,100),Vector2D(0,-100),true,parent);
    bullet->Update(nullptr);
    auto collision=FindCollision(*bullet,*enemy);
    Check(collision.hit && collision.pointA,"swept bullet precise hit");
    bullet->CollisionEnter(enemy,*collision.pointA); enemy->CollisionEnter(bullet);
    Check(!enemy->IsAlive() && bullet->IsAlive() && bullet->GetShapes().empty(),"bullet explosion remains without collision shapes");
    // More than one contact chooses the point with largest local y.
    bullet->CollisionEnter(enemy,{0,90});
    bullet->CollisionEnter(enemy,{0,20});
    Check(Near(bullet->GetBounds().topLeft.y,84),"largest-y impact");
    for (int i=0;i<8;++i) bullet->Update(nullptr);
    Check(!bullet->IsAlive(),"explosion expires");
    auto child=std::make_shared<Bullet>(Point2D{},Vector2D(0,10),false,parent);
    parent->Kill(); Check(!child->IsAlive(),"dead parent kills bullet");
    parent=std::make_shared<GameObject>();
    child=std::make_shared<Bullet>(Point2D{},Vector2D(0,10),false,parent);
    parent.reset(); Check(!child->IsAlive(),"expired parent kills bullet");
    auto armored=std::make_shared<Enemy2>(Point2D{},std::vector<Point2D>{{},{},{},{}});
    auto shot=std::make_shared<Bullet>(Point2D{},Vector2D{},true,armored);
    armored->CollisionEnter(shot); Check(armored->IsAlive() && armored->GetHealth()==1,"two health enemy");
    armored->CollisionEnter(shot); Check(!armored->IsAlive(),"second hit destroys armor");
}
}
int main() {
    try { MathTests(); CollisionTests(); BezierTests(); NotificationTests(); BulletEnemyTests();
        std::cout<<"Core tests passed\n";
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
