#include "Galaga.h"
#include <algorithm>
#include <array>
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
constexpr char kKeySetup[] = "galaga.key.setup";
constexpr char kRawKeyPrefix[] = "galaga.raw.key.";
constexpr int kReplayCoinGraceFrames = 150;

//-----------------------------------------------------------------------
/**
 * @brief Builds the notification name for a raw keyboard scan code.
 * @param key Physical key scan code.
 * @return Notification name used by the keyboard setup screen.
 */
//-----------------------------------------------------------------------
std::string RawKeyNotification(unsigned int key)
{
    return std::string(kRawKeyPrefix) + std::to_string(key);
}

//-----------------------------------------------------------------------
/**
 * @brief Converts a keyboard scan code into a display-friendly name.
 * @param key Physical key scan code.
 * @return A readable key label, or a numeric fallback for unsupported keys.
 */
//-----------------------------------------------------------------------
std::string KeyName(unsigned int key)
{
    static constexpr std::array<const char*, 26> letters = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
    };
    static constexpr std::array<const char*, 10> digits = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"
    };

    if (key < letters.size()) {
        return letters[key];
    }
    if (key >= CMPUT350::Key::Num1 && key <= CMPUT350::Key::Num0) {
        return digits[key - CMPUT350::Key::Num1];
    }

    switch (key) {
        case CMPUT350::Key::Space: return "Space";
        case CMPUT350::Key::Left: return "Left";
        case CMPUT350::Key::Right: return "Right";
        case CMPUT350::Key::Up: return "Up";
        case CMPUT350::Key::Down: return "Down";
        case CMPUT350::Key::Enter: return "Enter";
        case CMPUT350::Key::Escape: return "Escape";
        case CMPUT350::Key::F1: return "F1";
        case CMPUT350::Key::F2: return "F2";
        case CMPUT350::Key::F3: return "F3";
        case CMPUT350::Key::F4: return "F4";
        case CMPUT350::Key::F5: return "F5";
        case CMPUT350::Key::F6: return "F6";
        case CMPUT350::Key::F7: return "F7";
        case CMPUT350::Key::F8: return "F8";
        case CMPUT350::Key::F9: return "F9";
        case CMPUT350::Key::F10: return "F10";
        case CMPUT350::Key::F11: return "F11";
        case CMPUT350::Key::F12: return "F12";
        default: return "Key " + std::to_string(key);
    }
}
}

//-----------------------------------------------------------------------
/**
 * @brief Creates a game manager for the given window dimensions.
 * @param width Playfield width in pixels.
 * @param height Playfield height in pixels.
 */
//-----------------------------------------------------------------------
Galaga::Galaga(unsigned int width, unsigned int height):
    mAlive(true),
    mWidth(width),
    mHeight(height),
    mScore(0),
    mLevel(0),
    mEnemyCount(0),
    mExtraLives(2),
    mCoins(0),
    mPlayerAlive(false),
    mResetting(false),
    mStateStartFrame(-1),
    mLastAttackElapsed(0),
    mAttackCounter(0),
    mWavesSpawned(0),
    mRespawnFrame(-1),
    state(GameState::Title),
    mEngine(nullptr),
    mNotifications(nullptr),
    mBindings{CMPUT350::Key::A, CMPUT350::Key::D, CMPUT350::Key::Space},
    mSetupSelection(0),
    mWaitingForKey(false),
    mCustomBindings(false),
    mSetupStatus(),
    mReplaying(false),
    mReplayDone(false)
{
}

//-----------------------------------------------------------------------
/**
 * @brief Registers game notifications, installs controls, and creates the starfield.
 * @param context The engine's initialization and per-frame context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::Initialize(CMPUT350::GameContext* context)
{
    // The engine sets CurrObject to this game manager before Initialize.
    const std::weak_ptr<CMPUT350::GameObject> self = context->CurrObject;

    if (self.expired() || context->mNotificationManager == nullptr
        || context->mEngineView == nullptr) {
        return;
    }

    mNotifications = context->mNotificationManager;
    context->mNotificationManager->Register(self, kCoin);
    context->mNotificationManager->Register(self, kStart);
    context->mNotificationManager->Register(self, kKeySetup);
    context->mNotificationManager->Register(self, Enemy::kDestroyedNotification);
    context->mNotificationManager->Register(self, Player::kDestroyedNotification);

    // Remember the engine so ReceiveNotification can find a just-killed enemy.
    mEngine = context->mEngineView;
    InstallBindings();

    context->mEngineView->AddGameObject(std::make_shared<Stars>(250, CMPUT350::Rect(0, 0, 768, 1024)));
}

//-----------------------------------------------------------------------
/**
 * @brief Installs coin, start, setup, and player-control key bindings.
 *
 * Arrow keys remain available until a custom movement binding is saved.
 * Implemented with the help of AI.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::InstallBindings()
{
    if (mEngine == nullptr) {
        return;
    }

    mEngine->ClearKeyNotifications();
    mEngine->InstallKeyDownNotification(CMPUT350::Key::Num5, kCoin);
    mEngine->InstallKeyDownNotification(CMPUT350::Key::Num1, kStart);
    mEngine->InstallKeyDownNotification(CMPUT350::Key::F1, kKeySetup);

    const int leftKey = static_cast<int>(mBindings[static_cast<std::size_t>(BindAction::MoveLeft)]);
    const int rightKey = static_cast<int>(mBindings[static_cast<std::size_t>(BindAction::MoveRight)]);
    const int fireKey = static_cast<int>(mBindings[static_cast<std::size_t>(BindAction::Fire)]);

    mEngine->InstallKeyDownNotification(leftKey, Player::kMoveLeftDown);
    mEngine->InstallKeyUpNotification(leftKey, Player::kMoveLeftUp);
    mEngine->InstallKeyDownNotification(rightKey, Player::kMoveRightDown);
    mEngine->InstallKeyUpNotification(rightKey, Player::kMoveRightUp);
    mEngine->InstallKeyDownNotification(fireKey, Player::kShoot);

    if (!mCustomBindings) {
        mEngine->InstallKeyDownNotification(CMPUT350::Key::Left, Player::kMoveLeftDown);
        mEngine->InstallKeyUpNotification(CMPUT350::Key::Left, Player::kMoveLeftUp);
        mEngine->InstallKeyDownNotification(CMPUT350::Key::Right, Player::kMoveRightDown);
        mEngine->InstallKeyUpNotification(CMPUT350::Key::Right, Player::kMoveRightUp);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Routes every supported key press to the keyboard setup screen.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::InstallKeySetupBindings()
{
    if (mEngine == nullptr) {
        return;
    }

    mEngine->ClearKeyNotifications();
    for (unsigned int key = 0; key < CMPUT350::Key::ScanCount; ++key) {
        mEngine->InstallKeyDownNotification(
            static_cast<int>(key),
            RawKeyNotification(key)
        );
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Processes a key press while assigning player controls.
 *
 * Reserved and duplicate keys are rejected; Escape cancels an assignment or
 * exits setup.
 * @param key Physical key scan code received from the engine.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::HandleKeySetupKey(unsigned int key)
{
    if (key == CMPUT350::Key::Escape) {
        if (mWaitingForKey) {
            mWaitingForKey = false;
            mSetupStatus = "Key assignment cancelled.";
        } else {
            EnterState(GameState::Title);
            InstallBindings();
        }
        return;
    }

    if (!mWaitingForKey) {
        if (key >= CMPUT350::Key::Num1 && key <= CMPUT350::Key::Num3) {
            mSetupSelection = static_cast<int>(key - CMPUT350::Key::Num1);
            mWaitingForKey = true;
            static constexpr std::array<const char*, 3> actionNames = {
                "move left", "move right", "fire"
            };
            mSetupStatus = std::string("Press a key for ") +
                actionNames[static_cast<std::size_t>(mSetupSelection)] + ".";
        }
        return;
    }

    if (key == CMPUT350::Key::Num1 || key == CMPUT350::Key::Num5 ||
        key == CMPUT350::Key::F1) {
        mSetupStatus = "That key is reserved for the title screen.";
        return;
    }

    for (std::size_t i = 0; i < mBindings.size(); ++i) {
        if (i != static_cast<std::size_t>(mSetupSelection) && mBindings[i] == key) {
            mSetupStatus = "That key is already assigned.";
            return;
        }
    }

    mBindings[static_cast<std::size_t>(mSetupSelection)] = key;
    mCustomBindings = true;
    mWaitingForKey = false;
    mSetupStatus = "Binding saved. Choose another action or press Escape.";
}

//-----------------------------------------------------------------------
/**
 * @brief Resets gameplay data and begins a new recorded session.
 *
 * Implemented with the help of AI.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::StartNewGame()
{
    ClearPlayfield();
    mScore = 0;
    mLevel = 0;
    mEnemyCount = 0;
    mExtraLives = 2;
    mPlayerAlive = false;
    mRespawnFrame = -1;
    mLastAttackElapsed = 0;
    mAttackCounter = 0;
    mWavesSpawned = 0;
    EnterState(GameState::Startup);
}

//-----------------------------------------------------------------------
/**
 * @brief Starts the recorded demo after the title-screen coin grace period.
 *
 * Only start and player-control notifications are replayed. If no valid
 * recording exists, replay is marked complete instead.
 * Implemented with the help of AI.
 * @param context The current engine context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::BeginReplay(CMPUT350::GameContext* context)
{
    if (context == nullptr || mNotifications == nullptr || state != GameState::Title) {
        return;
    }

    const auto& events = mNotifications->GetRecordedEvents();
    const bool hasStartEvent = std::any_of(
        events.begin(),
        events.end(),
        [](const CMPUT350::NotificationManager::RecordedEvent& event) {
            return event.message == kStart;
        }
    );
    if (!hasStartEvent) {
        mReplayDone = true;
        return;
    }

    const bool started = mNotifications->StartReplay(
        [](const std::string& message) {
            return message == kStart ||
                message == Player::kMoveLeftDown ||
                message == Player::kMoveLeftUp ||
                message == Player::kMoveRightDown ||
                message == Player::kMoveRightUp ||
                message == Player::kShoot;
        }
    );
    mReplayDone = true;
    if (started) {
        mCoins = 1;
        mReplaying = true;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Advances the game state and schedules level gameplay.
 *
 * Coordinates title replay, enemy wave entry, attack timing, and safe
 * respawning. Implemented with the help of AI.
 * @param context The current frame, engine, and rendering context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::Update(CMPUT350::GameContext* context)
{
    if (mStateStartFrame < 0) {
        mStateStartFrame = context->currentFrame;
    }

    switch (state) {
        case GameState::Title:
            if (mCoins == 0 && !mReplaying && !mReplayDone &&
                context->currentFrame - mStateStartFrame >= kReplayCoinGraceFrames) {
                BeginReplay(context);
            }
            break;

        case GameState::KeySetup:
            break;

        case GameState::Startup:

            // If no player exists spawn player
            if (mPlayerAlive == false) {
                SpawnPlayer(context);
            }

            // After 3 seconds enter Waves state
            if (context->currentFrame - mStateStartFrame >= 90) {
                this->EnterState(GameState::Waves);
            }

            break;

        case GameState::Waves: {
            constexpr int waveInterval = 90;
            const int elapsed = context->currentFrame - mStateStartFrame;

            while (mWavesSpawned < 5 &&
                elapsed >= 30 + waveInterval * mWavesSpawned) {
                SpawnWave(mWavesSpawned, context);
                ++mWavesSpawned;
            }

            // Do not begin attacks until every living ship has finished its
            // entry path, even if the last wave takes longer than its interval.
            if (mWavesSpawned == 5 && elapsed >= 30 + 5 * waveInterval &&
                AllEnemiesInFormation(context)) {
                mLastAttackElapsed = context->currentFrame;
                this->EnterState(GameState::Gameplay);
            }

            break; 
        }
        case GameState::Gameplay: {
            constexpr int attackInterval = 300;
            if (mEnemyCount > 0 && mPlayerAlive &&
                context->currentFrame - mLastAttackElapsed >= attackInterval) {
                SelectAttacks(context);
                mLastAttackElapsed = context->currentFrame;
            }

            TryRespawn(context);

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

//-----------------------------------------------------------------------
/**
 * @brief Adds a player ship at the bottom-center home position.
 * @param context The engine context used to queue the player object.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::SpawnPlayer(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->mEngineView == nullptr) {
        return;
    }

    context->mEngineView->AddGameObject(
        std::make_shared<Player>(CMPUT350::Point2D(mWidth / 2.0f, 900.0f))
    );
    mPlayerAlive = true;
}

//-----------------------------------------------------------------------
/**
 * @brief Selects up to three formation enemies to launch attacks.
 *
 * Attack selection is deterministic; every fourth group can include an
 * armored enemy's capture attempt when an active player is available.
 * Implemented with the help of AI.
 * @param context The current game context used to target and launch attacks.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::SelectAttacks(CMPUT350::GameContext* context)
{
    if (context == nullptr || mEngine == nullptr || !mPlayerAlive) {
        return;
    }

    std::vector<std::shared_ptr<Enemy>> candidates;
    for (const auto& object : *mEngine) {
        auto enemy = std::dynamic_pointer_cast<Enemy>(object);
        if (enemy != nullptr && enemy->IsAlive() && enemy->IsInFormation()) {
            candidates.push_back(std::move(enemy));
        }
    }
    if (candidates.empty()) {
        return;
    }

    auto player = PlayerTarget(context);
    const std::size_t start =
        static_cast<std::size_t>(mAttackCounter) % candidates.size();
    const std::size_t attackCount = std::min<std::size_t>(3, candidates.size());
    std::vector<bool> selected(candidates.size(), false);
    std::size_t launched = 0;

    // Every fourth attack group includes an armored capture attempt when a
    // controllable player ship and an armored enemy are both available.
    if (mAttackCounter % 4u == 3u) {
        bool playerCanBeCaptured = false;
        for (const auto& object : *mEngine) {
            const auto candidatePlayer = std::dynamic_pointer_cast<Player>(object);
            if (candidatePlayer != nullptr && candidatePlayer->IsAlive() &&
                candidatePlayer->IsActive()) {
                playerCanBeCaptured = true;
                player = candidatePlayer->GetLocation();
                break;
            }
        }

        if (playerCanBeCaptured) {
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                auto armored = std::dynamic_pointer_cast<EnemyArmored>(candidates[i]);
                if (armored != nullptr) {
                    armored->BeginCapture();
                    if (armored->Attack(context, player)) {
                        selected[i] = true;
                        ++launched;
                    }
                    break;
                }
            }
        }
    }

    for (std::size_t offset = 0; offset < candidates.size() &&
        launched < attackCount; ++offset) {
        const std::size_t index = (start + offset) % candidates.size();
        if (selected[index]) {
            continue;
        }
        if (candidates[index]->Attack(context, player)) {
            selected[index] = true;
            ++launched;
        }
    }

    ++mAttackCounter;
}

//-----------------------------------------------------------------------
/**
 * @brief Finds the living player's position, or returns the default home point.
 * @param context The current game context.
 * @return The active player's location or the playfield's bottom-center point.
 */
//-----------------------------------------------------------------------
CMPUT350::Point2D Galaga::PlayerTarget(CMPUT350::GameContext* context)
{
    if (context != nullptr && mEngine != nullptr) {
        for (const auto& object : *mEngine) {
            const auto player = std::dynamic_pointer_cast<Player>(object);
            if (player != nullptr && player->IsAlive()) {
                return player->GetLocation();
            }
        }
    }

    return CMPUT350::Point2D(mWidth / 2.0f, 900.0f);
}

//-----------------------------------------------------------------------
/**
 * @brief Checks whether all living enemies have returned to formation.
 *
 * Implemented with the help of AI.
 * @param context The engine context used to inspect active game objects.
 * @return true if every living enemy is in formation; otherwise false.
 */
//-----------------------------------------------------------------------
bool Galaga::AllEnemiesInFormation(CMPUT350::GameContext* context)
{
    if (context == nullptr || mEngine == nullptr) {
        return false;
    }

    for (auto& object : *mEngine) {
        auto enemy = std::dynamic_pointer_cast<Enemy>(object);
        if (enemy != nullptr && enemy->IsAlive() && !enemy->IsInFormation()) {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------
/**
 * @brief Respawns a reserve ship after the delay and formation safety checks.
 * @param context The current frame and engine context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::TryRespawn(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->mEngineView == nullptr ||
        mPlayerAlive || mExtraLives == 0) {
        return;
    }

    if (mRespawnFrame < 0) {
        mRespawnFrame = context->currentFrame;
    }

    constexpr int respawnDelay = 300;
    if (context->currentFrame - mRespawnFrame < respawnDelay || !AllEnemiesInFormation(context)) {
        return;
    }

    SpawnPlayer(context);
    --mExtraLives;
    mRespawnFrame = -1;
}

void Galaga::LateUpdate(CMPUT350::GameContext* context)
{
    
}

//-----------------------------------------------------------------------
/**
 * @brief Draws the title, keyboard setup, and gameplay status overlays.
 * @param context The engine context containing the GUI drawing context.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::RenderUI(CMPUT350::GameContext* context)
{
    if (context == nullptr || context->GUIContext == nullptr) {
        return;
    }

    // Draw coin count
    std::string coinText = "Coins: ";
    context->GUIContext->DrawCenteredText(coinText + std::to_string(mCoins), 14, CMPUT350::Point2D(mWidth / 2, mHeight - 16), CMPUT350::Colors::white);

    if (state == GameState::KeySetup) {
        context->GUIContext->DrawCenteredText(
            "Keyboard Setup",
            48,
            CMPUT350::Point2D(mWidth / 2, 160),
            CMPUT350::Colors::white
        );

        static constexpr std::array<const char*, 3> actionNames = {
            "Move Left", "Move Right", "Fire"
        };
        for (std::size_t i = 0; i < mBindings.size(); ++i) {
            const std::string line = std::to_string(i + 1) + ". " +
                actionNames[i] + ": " + KeyName(mBindings[i]);
            context->GUIContext->DrawCenteredText(
                line,
                24,
                CMPUT350::Point2D(mWidth / 2, 300 + 55 * static_cast<float>(i)),
                CMPUT350::Colors::white
            );
        }

        const std::string prompt = mWaitingForKey
            ? "Press a key to assign, or Escape to cancel."
            : "Press 1, 2, or 3 to select an action. Escape returns.";
        context->GUIContext->DrawCenteredText(
            prompt,
            18,
            CMPUT350::Point2D(mWidth / 2, 530),
            CMPUT350::Colors::white
        );
        if (!mSetupStatus.empty()) {
            context->GUIContext->DrawCenteredText(
                mSetupStatus,
                18,
                CMPUT350::Point2D(mWidth / 2, 575),
                CMPUT350::Colors::yellow
            );
        }
        return;
    }

    if (state == GameState::Title) {
        // Draw title screen text
        context->GUIContext->DrawCenteredText("Galuga", 100, CMPUT350::Point2D(mWidth / 2, mHeight / 2 - 50), CMPUT350::Colors::white);

        if (context->currentFrame % 30 < 15) {
            if (mCoins == 0) {
                context->GUIContext->DrawCenteredText("Insert Coin [num 5]", 20, CMPUT350::Point2D(mWidth / 2, mHeight / 2 + 50), CMPUT350::Colors::white);
            } else {
                context->GUIContext->DrawCenteredText("Start Game [num 1]", 20, CMPUT350::Point2D(mWidth / 2, mHeight / 2 + 50), CMPUT350::Colors::white);
            }
        }
        context->GUIContext->DrawCenteredText(
            "F1: Keyboard Setup",
            16,
            CMPUT350::Point2D(mWidth / 2, mHeight / 2 + 100),
            CMPUT350::Colors::white
        );

    } else {
        // Draw score and extra lives
        std::string scoreText = "Score: ";
        context->GUIContext->DrawText(scoreText + std::to_string(mScore), 18, CMPUT350::Point2D(15, 15), CMPUT350::Colors::white);

        std::string livesText = "Lives: ";
        const unsigned int remainingLives = mExtraLives + (mPlayerAlive ? 1u : 0u);
        context->GUIContext->DrawText(livesText + std::to_string(remainingLives), 18, CMPUT350::Point2D(mWidth - 160, 15), CMPUT350::Colors::white);

    }

    if (state == GameState::Startup) {
        // Draw level number
        std::string levelText = "Level ";
        context->GUIContext->DrawCenteredText(levelText + std::to_string(mLevel), 40, CMPUT350::Point2D(mWidth / 2, mHeight / 2), CMPUT350::Colors::white);
    }

    if (state == GameState::GameOver) {
        // Draw game over text
        context->GUIContext->DrawCenteredText("Game Over", 40, CMPUT350::Point2D(mWidth / 2, mHeight / 2), CMPUT350::Colors::white);
    }

}

//-----------------------------------------------------------------------
/**
 * @brief Handles coin, start, setup, and object-destruction notifications.
 * @param notification Name of the notification being delivered.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::ReceiveNotification(const std::string& notification)
{
    if (state == GameState::KeySetup) {
        for (unsigned int key = 0; key < CMPUT350::Key::ScanCount; ++key) {
            if (notification == RawKeyNotification(key)) {
                HandleKeySetupKey(key);
                return;
            }
        }
        return;
    }

    if (notification == kKeySetup) {
        if (state == GameState::Title) {
            mSetupSelection = 0;
            mWaitingForKey = false;
            mSetupStatus.clear();
            EnterState(GameState::KeySetup);
            InstallKeySetupBindings();
        }
        return;
    }

    if (notification == kStart) { // Start game

        if (state == GameState::Title && mCoins > 0) {
            --mCoins;
            StartNewGame();
        }

    } else if (notification == kCoin) { // Insert coin

        if (mReplaying) {
            if (mNotifications != nullptr) {
                mNotifications->StopReplay();
            }
            mReplaying = false;
            ClearPlayfield();
            EnterState(GameState::Title);
            mReplayDone = true;
        }
        ++mCoins;
        if (state == GameState::Title && mNotifications != nullptr) {
            mNotifications->StartRecording();
            mReplayDone = true;
        }

    } else if (notification == Enemy::kDestroyedNotification) { // Enemy death

        if (mResetting) {
            return;
        }

        // Decrease enemy counter
        if (mEnemyCount > 0) {
            --mEnemyCount;
        }

        ClaimEnemyScore();

    } else if (notification == Player::kDestroyedNotification) { // Player death

        if (mResetting) {
            return;
        }

        mPlayerAlive = false;
        mRespawnFrame = -1;

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

//-----------------------------------------------------------------------
/**
 * @brief Changes the game state and performs that state's entry setup.
 * @param newState State to enter.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::EnterState(GameState newState)
{
    mStateStartFrame = -1;

    switch (newState) {
        case GameState::Title:
            ClearPlayfield();
            if (mNotifications != nullptr) {
                mNotifications->StopRecording();
                if (mReplaying) {
                    mNotifications->StopReplay();
                }
            }
            mReplaying = false;
            mReplayDone = false;
            mScore = 0;
            mLevel = 0;
            mEnemyCount = 0;
            mExtraLives = 2;
            mPlayerAlive = false;
            mRespawnFrame = -1;

            break;

        case GameState::Startup:
            
            ++mLevel;

            break;

        case GameState::Waves:
            mEnemyCount = 0;
            mWavesSpawned = 0;

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
 * @brief Removes active player, enemy, and bullet objects without scoring.
 *
 * Implemented with the help of AI.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::ClearPlayfield()
{
    if (mEngine == nullptr) {
        mPlayerAlive = false;
        mEnemyCount = 0;
        return;
    }

    mResetting = true;
    for (const auto& object : *mEngine) {
        if (auto player = std::dynamic_pointer_cast<Player>(object)) {
            player->Kill();
        } else if (auto enemy = std::dynamic_pointer_cast<Enemy>(object)) {
            enemy->Kill();
        } else if (auto bullet = std::dynamic_pointer_cast<Bullet>(object)) {
            bullet->Kill();
        }
    }
    mResetting = false;
    mPlayerAlive = false;
    mEnemyCount = 0;
    mRespawnFrame = -1;
}

//-----------------------------------------------------------------------
/**
 * @brief Adds points once for the most recently destroyed enemy.
 * @return None.
 */
//-----------------------------------------------------------------------
void Galaga::ClaimEnemyScore()
{
    if (mEngine == nullptr) {
        return;
    }

    for (const auto& object : *mEngine) {
        auto enemy = std::dynamic_pointer_cast<Enemy>(object);
        if (enemy != nullptr && !enemy->IsAlive()) {
            const int points = enemy->TakeScore();
            if (points > 0) {
                mScore += static_cast<unsigned int>(points);
                return;
            }
        }
    }
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
    mEnemyCount += static_cast<unsigned int>(kShipsPerWave);
}