#include "Galaga.h"
#include <memory>
#include <vector>
#include <string>
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
    mScore(0), mLevel(0), mEnemyCount(0), mExtraLives(3), mCoins(0), mStateStartFrame(0), mLastAttackElapsed(0), state(GameState::Title),
    mEngine(nullptr)
{
}

void Galaga::Initialize(CMPUT350::GameContext* context)
{
    // The engine sets CurrObject to this player before calling Initialize.
    const std::weak_ptr<CMPUT350::GameObject> self = context->CurrObject;

    if (self.expired() || context->mNotificationManager == nullptr
        || context->mEngineView == nullptr) {
        return;
    }

    context->mNotificationManager->Register(self, kCoin);
    context->mNotificationManager->Register(self, kStart);
    context->mNotificationManager->Register(self, Enemy::kDestroyedNotification);
    context->mNotificationManager->Register(self, Player::kDestroyedNotification);

    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::Num5, kCoin);
    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::Num1, kStart);

    // The manager owns the player's key bindings; the player only listens for
    // the resulting notifications. A/Left and D/Right are interchangeable.
    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::A, Player::kMoveLeftDown);
    context->mEngineView->InstallKeyUpNotification(CMPUT350::Key::A, Player::kMoveLeftUp);
    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::Left, Player::kMoveLeftDown);
    context->mEngineView->InstallKeyUpNotification(CMPUT350::Key::Left, Player::kMoveLeftUp);
    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::D, Player::kMoveRightDown);
    context->mEngineView->InstallKeyUpNotification(CMPUT350::Key::D, Player::kMoveRightUp);
    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::Right, Player::kMoveRightDown);
    context->mEngineView->InstallKeyUpNotification(CMPUT350::Key::Right, Player::kMoveRightUp);
    context->mEngineView->InstallKeyDownNotification(CMPUT350::Key::Space, Player::kShoot);

    // Remember the engine so ReceiveNotification can find a just-killed enemy.
    mEngine = context->mEngineView;

    context->mEngineView->AddGameObject(std::make_shared<Stars>(250, CMPUT350::Rect(0, 0, 768, 1024)));
}

void Galaga::Update(CMPUT350::GameContext* context)
{
    if (mStateStartFrame == 0) {
        mStateStartFrame = context->currentFrame;
    }

    switch (state) {
        case GameState::Title:
            break;

        case GameState::Startup:

            // If no player exists spawn player
            if (mPlayerAlive == false) {
                context->mEngineView->AddGameObject(std::make_shared<Player>(CMPUT350::Point2D(768 / 2, 900)));
                mPlayerAlive = true;
            }

            // After 3 seconds enter Waves state
            if (context->currentFrame - mStateStartFrame >= 90) {
                this->EnterState(GameState::Waves);
            }

            break;

        case GameState::Waves: {

            int spawnFreq = 90; // Interval between wave entries (30 frames = 1 sec)

            // Each entry launches one wave of 8 ships, which fly a Bezier path
            // from off-screen into their formation row. Five waves of 8 make the
            // 40-enemy formation set in EnterState().
            if (context->currentFrame - mStateStartFrame == 30) {
                SpawnWave(0, context);
            }

            if (context->currentFrame - mStateStartFrame == 30 + spawnFreq) {
                SpawnWave(1, context);
            }

            if (context->currentFrame - mStateStartFrame == 30 + 2 * spawnFreq) {
                SpawnWave(2, context);
            }

            if (context->currentFrame - mStateStartFrame == 30 + 3 * spawnFreq) {
                SpawnWave(3, context);
            }

            if (context->currentFrame - mStateStartFrame == 30 + 4 * spawnFreq) {
                SpawnWave(4, context);
            }

            // After the last wave enters, move on to gameplay. (TODO: May need
            // extra time to ensure the last wave is in formation first.)
            if (context->currentFrame - mStateStartFrame == 30 + 5 * spawnFreq) {
                this->EnterState(GameState::Gameplay);
            }

            break; 
        }
        case GameState::Gameplay: {

            int attackFreq = 300; // Interval between attacks (30 frames = 1 sec)
            int maxAttackLength = 300; // Length of time of longest possible attack

            // If an enemy exists AND if the player exists AND if interval since last attack has been reached: Activate attack
            if (mEnemyCount > 0 && mPlayerAlive == true && context->currentFrame - mLastAttackElapsed >= attackFreq) {

                // TODO: Attack with set of enemies (semi-randomized if possible)

                // Set frame of last attack
                mLastAttackElapsed = context->currentFrame;
            }

            // If player is dead AND player has an extra life AND time since last attack is greater than max attack length: Spawn player
            if (mPlayerAlive == false && mExtraLives > 0 && context->currentFrame - mLastAttackElapsed >= maxAttackLength) {
                context->mEngineView->AddGameObject(std::make_shared<Player>(CMPUT350::Point2D(768 / 2, 900)));
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

            if (context->currentFrame - mStateStartFrame >= 300) {
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
    std::string coinText = "Coins: " + std::to_string(mCoins);
    context->GUIContext->DrawCenteredText(coinText, 18, CMPUT350::Point2D(mWidth / 2, mHeight - 20), CMPUT350::Colors::white);

    if (state == GameState::Title) {
        context->GUIContext->DrawCenteredText("GALAGA", 96, CMPUT350::Point2D(mWidth / 2, mHeight / 2 - 60), CMPUT350::Colors::yellow);
        if (context->currentFrame % 40 < 20) {
            if (mCoins == 0) {
                context->GUIContext->DrawCenteredText("INSERT COIN [5]", 24, CMPUT350::Point2D(mWidth / 2, mHeight / 2 + 60), CMPUT350::Colors::white);
            } else {
                context->GUIContext->DrawCenteredText("PRESS 1 TO START", 24, CMPUT350::Point2D(mWidth / 2, mHeight / 2 + 60), CMPUT350::Colors::white);
            }
        }
    } else {
        context->GUIContext->DrawText("SCORE: " + std::to_string(mScore), 20, CMPUT350::Point2D(16, 16), CMPUT350::Colors::white);
        context->GUIContext->DrawText("LIVES: " + std::to_string(mExtraLives), 20, CMPUT350::Point2D(mWidth - 150, 16), CMPUT350::Colors::white);
    }

    if (state == GameState::Startup) {
        context->GUIContext->DrawCenteredText("LEVEL " + std::to_string(mLevel), 48, CMPUT350::Point2D(mWidth / 2, mHeight / 2), CMPUT350::Colors::yellow);
    }

    if (state == GameState::GameOver) {
        context->GUIContext->DrawCenteredText("GAME OVER", 60, CMPUT350::Point2D(mWidth / 2, mHeight / 2), CMPUT350::Colors::red);
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

    } else if (notification == Enemy::kDestroyedNotification) { // Enemy death

        // Decrease enemy counter
        if (mEnemyCount > 0) {
            --mEnemyCount;
        }

        // The notification carries no payload, so find the enemy that just died
        // and claim its score. TakeScore() returns 0 once a kill is counted, so
        // multiple same-frame deaths are each counted exactly once.
        if (mEngine != nullptr) {
            for (auto& object : *mEngine) {
                auto enemy = std::dynamic_pointer_cast<Enemy>(object);
                if (enemy != nullptr && !enemy->IsAlive()) {
                    const int points = enemy->TakeScore();
                    if (points > 0) {
                        mScore += points;
                        break;
                    }
                }
            }
        }

    } else if (notification == Player::kDestroyedNotification) { // Player death

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
    mStateStartFrame = 0;

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

//-----------------------------------------------------------------------
/**
 * @brief Launches one wave of eight enemies into its formation row.
 *
 * Every enemy is given a cubic Bezier entry path (four control points: an
 * off-screen start, two handles, and the formation slot) so the engine flies it
 * into place at a constant rate before it settles into the formation. Even
 * waves sweep in from the left edge and odd waves from the right, reproducing
 * the alternating entry of the arcade game. The back rows hold the tougher
 * enemy types, matching their higher score values.
 *
 * @param wave Zero-based row being filled (0 is the top/back row).
 * @param context The engine's per-frame context.
 * @return None. The queued enemies are activated on the next frame.
 */
//-----------------------------------------------------------------------
void Galaga::SpawnWave(int wave, CMPUT350::GameContext* context)
{
    if (context == nullptr || context->mEngineView == nullptr) {
        return;
    }

    constexpr int kShipsPerWave = 8;
    constexpr float kColumnSpacing = 84.0f;
    constexpr float kRowSpacing = 80.0f;
    constexpr float kFormationLeft = 90.0f;
    constexpr float kFormationTop = 120.0f;

    const float rowY = kFormationTop + kRowSpacing * static_cast<float>(wave);
    const bool fromLeft = (wave % 2 == 0);

    for (int col = 0; col < kShipsPerWave; ++col) {
        const CMPUT350::Point2D slot(
            kFormationLeft + kColumnSpacing * static_cast<float>(col),
            rowY
        );

        std::vector<CMPUT350::Point2D> entryPath;
        if (fromLeft) {
            entryPath = {
                CMPUT350::Point2D(-160.0f, slot.y - 260.0f),
                CMPUT350::Point2D(slot.x - 420.0f, slot.y - 360.0f),
                CMPUT350::Point2D(slot.x - 150.0f, slot.y - 80.0f),
                slot
            };
        } else {
            entryPath = {
                CMPUT350::Point2D(928.0f, slot.y - 260.0f),
                CMPUT350::Point2D(slot.x + 420.0f, slot.y - 360.0f),
                CMPUT350::Point2D(slot.x + 150.0f, slot.y - 80.0f),
                slot
            };
        }

        std::shared_ptr<Enemy> enemy;
        if (wave == 0) {
            enemy = std::make_shared<EnemyArmored>(slot, entryPath);
        } else if (wave == 1) {
            enemy = std::make_shared<EnemyWeaver>(slot, entryPath);
        } else {
            enemy = std::make_shared<EnemyScout>(slot, entryPath);
        }

        context->mEngineView->AddGameObject(enemy);
    }
}