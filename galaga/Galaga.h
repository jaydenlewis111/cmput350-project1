#ifndef GALAGA_H
#define GALAGA_H
#include "GameObject.h"
#include <memory>
class Player;
class Galaga : public CMPUT350::GameObject {
public:
    enum class State { Waiting, Startup, Waves, Playing, GameOver };
    Galaga(int width,int height);
    void Initialize(CMPUT350::GameContext *context) override;
    void Update(CMPUT350::GameContext *context) override;
    void RenderUI(CMPUT350::GameContext *context) override;
    void ReceiveNotification(const std::string &message) override;
    // Read-only state is useful for a HUD, tests, or a future attract-mode view.
    State GetState() const;
    int GetScore() const;
    int GetLives() const;
    int GetLevel() const;
    int GetCredits() const;
    int GetRemainingEnemies() const;
private:
    void Enter(State state,int frame);
    void ClearRound(CMPUT350::GameContext *context);
    void SpawnPlayer(CMPUT350::GameContext *context);
    void SpawnEnemy(CMPUT350::GameContext *context);
    bool AllInFormation(CMPUT350::GameContext *context) const;
    void ManageRespawn(CMPUT350::GameContext *context);
    int mWidth,mHeight,mCredits=0,mScore=0,mLives=3,mLevel=1;
    int mStateFrame=0,mSpawned=0,mRemaining=0,mNextSpawn=0,mNextAttack=0,mAttackCursor=0,mRespawnAt=-1;
    bool mStartRequested=false,mPlayerLost=false;
    State mState=State::Waiting;
    std::weak_ptr<Player> mPlayer;
};
#endif
