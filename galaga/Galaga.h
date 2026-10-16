#ifndef GALAGA_H
#define GALAGA_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "GameObject.h"
#include "GameContext.h"

//-----------------------------------------------------------------------
/**
 * @brief The phases of a Galaga game.
 *
 * The manager is a finite state machine driven by this enum. Title waits for a
 * coin and a start press; Startup shows the level number and spawns the player;
 * Waves flies the enemy formation in; Gameplay runs the fight; GameOver shows
 * the closing message; KeySetup edits the key bindings.
 */
//-----------------------------------------------------------------------
enum class GameState
{
    Title,     ///< Attract screen: insert coins and start, or open key setup.
    Startup,   ///< Level banner and player entry.
    Waves,     ///< Enemy ships fly in along their Bezier curves.
    Gameplay,  ///< The player fights the formation.
    GameOver,  ///< End-of-game message before returning to the title.
    KeySetup   ///< Re-assign the player's movement and fire keys.
};

//-----------------------------------------------------------------------
/**
 * @brief Owns and drives the whole Galaga game.
 *
 * Adding a single Galaga object to the engine is enough to start the game: it
 * creates the starfield, the player, and every wave of enemies, keeps the score
 * and lives, and moves between the states in GameState.
 *
 * The class listens to the notification manager for the coin, start, navigation
 * and raw-key messages (used by the key setup screen) as well as the player and
 * enemy destruction messages that keep the score and enemy count correct.
 *
 * Extra credit implemented here:
 * - a keyboard setup screen (GameState::KeySetup) that re-assigns bindings,
 * - a replay system that re-plays just the recorded input notifications,
 * - ship capture is driven from SelectAttacks via Enemy::BeginCapture.
 */
//-----------------------------------------------------------------------
class Galaga : public CMPUT350::GameObject
{
public:
    /// @brief Creates the manager for a window of the given size.
    Galaga(unsigned int width, unsigned int height);

    // GameObject Functions
    void Initialize(CMPUT350::GameContext* context) override;
    void Update(CMPUT350::GameContext* context) override;
    void LateUpdate(CMPUT350::GameContext* context) override;
    void RenderUI(CMPUT350::GameContext* context) override;
    void ReceiveNotification(const std::string& notification) override;
    bool IsAlive() const override;
    void Kill() override;

    /// @brief Transitions to a new state, running that state's entry setup.
    void EnterState(GameState newState);

private:
    /// @brief The player actions whose keys can be re-assigned.
    enum class BindAction
    {
        MoveLeft = 0,
        MoveRight = 1,
        Fire = 2,
        Count = 3
    };

    // ---- Setup ---------------------------------------------------------
    /// @brief Re-installs every key binding from the current mBindings table.
    void InstallBindings();
    /// @brief Begins a fresh, recorded game.
    void StartNewGame();
    /// @brief Starts a replay of the recorded game.
    void BeginReplay(CMPUT350::GameContext* context);

    // ---- Spawning and cleanup -----------------------------------------
    /// @brief Spawns one wave of 8 ships that fly into their formation row.
    void SpawnWave(int wave, CMPUT350::GameContext* context);
    /// @brief Adds a player ship at the home row.
    void SpawnPlayer(CMPUT350::GameContext* context);
    /// @brief Removes the player, enemies and bullets without scoring them.
    void ClearPlayfield();

    // ---- Gameplay helpers ---------------------------------------------
    /// @brief Launches a deterministic group of enemies, occasionally a capturer.
    void SelectAttacks(CMPUT350::GameContext* context);
    /// @brief Returns the live player's position, or the default home point.
    CMPUT350::Point2D PlayerTarget(CMPUT350::GameContext* context);
    /// @brief True when no live enemy is away from its formation slot.
    bool AllEnemiesInFormation(CMPUT350::GameContext* context);
    /// @brief Respawns the player after a death once the field is safe.
    void TryRespawn(CMPUT350::GameContext* context);
    /// @brief Claims the score for the most recently killed enemy.
    void ClaimEnemyScore();

    // ---- UI ------------------------------------------------------------
    void RenderTitle(CMPUT350::GameContext* context);
    void RenderHud(CMPUT350::GameContext* context);
    void RenderKeySetup(CMPUT350::GameContext* context);
    void RenderGameOver(CMPUT350::GameContext* context);

    bool mAlive;
    unsigned int mWidth;
    unsigned int mHeight;

    unsigned int mScore;       ///< Points earned this game.
    unsigned int mLevel;       ///< Current level number (starts at 1).
    unsigned int mExtraLives;  ///< Reserve ships left after the one in play.
    unsigned int mEnemyCount;  ///< Enemies still alive in the current level.
    unsigned int mCoins;       ///< Coins inserted but not yet used.

    bool mPlayerAlive;         ///< Whether a player ship is currently in play.
    bool mResetting;           ///< True while the playfield is being cleared.

    int mStateStartFrame;      ///< Frame the current state began (-1 until set).
    int mLastAttackElapsed;    ///< Elapsed frame of the last attack launched.
    unsigned int mAttackCounter; ///< Attacks launched, for deterministic choice.
    int mWavesSpawned;         ///< Number of enemy waves launched this level.
    int mRespawnFrame;         ///< Frame the player died (-1 when alive).

    GameState state;

    CMPUT350::EngineView* mEngine;                    ///< Cached engine view.
    CMPUT350::NotificationManager* mNotifications;    ///< Cached manager.

    /// @brief One scancode per BindAction.
    std::array<unsigned int, static_cast<std::size_t>(BindAction::Count)> mBindings;
    int mSetupSelection;   ///< Highlighted action in the key setup screen.
    bool mWaitingForKey;   ///< True while the next key press becomes a binding.

    bool mReplaying;       ///< True while a recorded game is being replayed.
    bool mReplayDone;      ///< True once the current recording has been shown.
};

#endif  // GALAGA_H
