#ifndef GALAGA_H
#define GALAGA_H

#include "GameObject.h"
#include "GameContext.h"

enum class GameState 
{
    Title, Startup, Waves, Gameplay, GameOver 
};

class Galaga : public CMPUT350::GameObject
{
public:
    Galaga(unsigned int width, unsigned int height);

    // GameObject Functions
    void Initialize(CMPUT350::GameContext* context) override;
    void Update(CMPUT350::GameContext* context) override;
    void LateUpdate(CMPUT350::GameContext* context) override;
    void RenderUI(CMPUT350::GameContext* context) override;
    void ReceiveNotification(const std::string& notification) override;
    bool IsAlive() const override;
    void Kill() override;

    // Galaga Functions
    void EnterState(GameState newState);

private:
    bool mAlive, mPlayerAlive;
    unsigned int mScore, mLevel, mEnemyCount, mExtraLives, mCoins, mStartFrame, mLastAttack, mWidth, mHeight;
    GameState state;
};
#endif // GALAGA_H