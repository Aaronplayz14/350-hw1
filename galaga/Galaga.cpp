#include "Galaga.h"
#include "Player.h"
#include "Enemy.h"
#include "Stars.h"
#include "NotificationManager.h"
#include "Keyboard.h"

namespace {
constexpr char kCoin[] = "galaga.coin";
constexpr char kStart[] = "galaga.start";
}

Galaga::Galaga(unsigned int width, unsigned int height) : mWidth(width), mHeight(height), mAlive(true), mPlayerAlive(false), 
    mScore(0), mLevel(0), mEnemyCount(0), mExtraLives(3), mCoins(0), mStartFrame(0), mLastAttack(0), state(GameState::Title)
{
}

void Galaga::Initialize(CMPUT350::GameContext* context)
{
    // The engine sets CurrObject to this player before calling Initialize.
    const std::weak_ptr<CMPUT350::GameObject> self = context->CurrObject;

    if (self.expired() || context->Notifications == nullptr
        || context->EngineContext == nullptr) {
        return;
    }

    context->Notifications->Register(self, kCoin);
    context->Notifications->Register(self, kStart);
    // TODO: Add enemy death and player death notif registrations

    context->EngineContext->InstallKeyDownNotification(CMPUT350::Key::Num5, kCoin);
    context->EngineContext->InstallKeyDownNotification(CMPUT350::Key::Num1, kStart);
    // TODO: Migrate all InstallKeyNotification commands from other game objects to this object

    context->EngineContext->AddGameObject(std::make_shared<Stars>(250, CMPUT350::Rect(0, 0, 768, 1024)));
}

void Galaga::Update(CMPUT350::GameContext* context)
{
    if (mStartFrame == 0) {
        mStartFrame = context->currentFrame;
    }

    switch (state) {
        case GameState::Title:
            break;

        case GameState::Startup:

            // If no player exists spawn player
            if (mPlayerAlive == false) {
                context->EngineContext->AddGameObject(std::make_shared<Player>(CMPUT350::Point2D(768 / 2, 900)));
                mPlayerAlive = true;
            }

            // After 3 seconds enter Waves state
            if (context->currentFrame - mStartFrame >= 90) {
                this->EnterState(GameState::Waves);
            }

            break;

        case GameState::Waves: {

            int spawnFreq = 90; // Interval between wave spawns (30 frames = 1 sec)
            auto* engine = context->EngineContext;

            // TODO: Update add enemy objects with entry paths and enemy types

            // After 1 second spawn Wave 1
            if (context->currentFrame - mStartFrame == 30) {
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 3, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 4, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 5, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 6, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 3, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 4, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 5, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 6, 200)));
            }

            // After 1 + x seconds spawn Wave 2
            if (context->currentFrame - mStartFrame == 30 + spawnFreq) {
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 2, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 7, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 2, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 7, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 2, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 7, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 3, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 6, 300)));
            }

            // After 1 + 2x seconds spawn Wave 3
            if (context->currentFrame - mStartFrame == 30 + 2 * spawnFreq) {
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 0, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 1, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 0, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 1, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 0, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 1, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 0, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 1, 400)));
            }

            // After 1 + 3x seconds spawn Wave 4
            if (context->currentFrame - mStartFrame == 30 + 3 * spawnFreq) {
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 8, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 9, 100)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 8, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 9, 200)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 8, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 9, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 8, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 9, 400)));
            }

            // After 1 + 4x seconds spawn Wave 5          
            if (context->currentFrame - mStartFrame == 30 + 4 * spawnFreq) {
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 4, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 5, 300)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 2, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 3, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 4, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 5, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 6, 400)));
                // engine->AddGameObject(std::make_shared<Enemy>(CMPUT350::Point2D(78 + 68 * 7, 400)));
            }

            // After 1 + 5x seconds enter Gameplay state (TODO: May need to add extra time to ensure last wave in formation before state change)
            if (context->currentFrame - mStartFrame == 30 + 5 * spawnFreq) {
                this->EnterState(GameState::Gameplay);
            }

            break; 
        }
        case GameState::Gameplay: {

            int attackFreq = 300; // Interval between attacks (30 frames = 1 sec)
            int maxAttackLength = 300; // Length of time of longest possible attack

            // If an enemy exists AND if the player exists AND if interval since last attack has been reached: Activate attack
            if (mEnemyCount > 0 && mPlayerAlive == true && context->currentFrame - mLastAttack >= attackFreq) {

                // TODO: Attack with set of enemies (semi-randomized if possible)

                // Set frame of last attack
                mLastAttack = context->currentFrame;
            }

            // If player is dead AND player has an extra life AND time since last attack is greater than max attack length: Spawn player
            if (mPlayerAlive == false && mExtraLives > 0 && context->currentFrame - mLastAttack >= maxAttackLength) {
                context->EngineContext->AddGameObject(std::make_shared<Player>(CMPUT350::Point2D(768 / 2, 900)));
                --mExtraLives;
                mPlayerAlive = true;
            }

            // If all enemies are dead enter Startup state for next level
            if (mEnemyCount == 0) {
                this->EnterState(GameState::Startup);
            }

            break;
        }
        case GameState::GameOver:

            if (context->currentFrame - mStartFrame >= 300) {
                this->EnterState(GameState::Title);
            }

            break;
        
        default:
            break;
    }
}

void Galaga::LateUpdate(CMPUT350::GameContext* context)
{
    
}

void Galaga::RenderUI(CMPUT350::GameContext* context)
{
    if (state == GameState::Title) {
        // TODO: Draw title screen text
    } else {
        // TODO: Draw score and extra lives
    }

    if (state == GameState::Startup) {
        // TODO: Draw level number
    }

    if (state == GameState::GameOver) {
        // TODO: Draw game over text
    }

}

void Galaga::ReceiveNotification(const std::string& notification)
{
    if (notification == kStart) { // Start game

        if (state == GameState::Title && mCoins > 0) {
            --mCoins;
            this->EnterState(GameState::Startup);
        }

    } else if (notification == kCoin) { // Insert coin

        ++mCoins;

    } else if (false) { // TODO: If enemy death

        // Decrease enemy counter
        --mEnemyCount;

        // TODO: Find enemy point value
        int points = 10; // Placeholder
        
        // Update score
        mScore += points; 

    } else if (false) { // TODO: If player death

        mPlayerAlive = false;

        if (mExtraLives == 0) {
            this->EnterState(GameState::GameOver);
        }
              
    }
}

bool Galaga::IsAlive() const
{
    return mAlive;
}

void Galaga::Kill()
{
}

void Galaga::EnterState(GameState newState)
{
    mStartFrame = 0;

    switch (newState) {
        case GameState::Title:

            // TODO: Remove remaining enemies
            
            mScore = 0;
            mLevel = 0;
            mEnemyCount = 0;
            mExtraLives = 3;

            break;

        case GameState::Startup:
            
            ++mLevel;

            break;

        case GameState::Waves:

            mEnemyCount = 40;

            break;

        case GameState::Gameplay:
            break;

        case GameState::GameOver:
            break;
        
        default:
            break;
    }

    state = newState;
}