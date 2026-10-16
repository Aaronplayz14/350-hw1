#include "GameEngine.h"
#include "Player.h"
#include "Enemy.h"
#include "Stars.h"
#include "Galaga.h"

//-----------------------------------------------------------------------
/**
 * @brief Builds the Galaga scene and runs the engine.
 *
 * Temporary scaffolding: the assignment expects main() to add a single Galaga
 * game object that owns the player, the enemies, the score, and the state
 * machine. That class now exists but is still being completed, so this harness
 * wires up a playable scene directly (player, star field, and a 5x8 grid of
 * enemies) so the player/bullet/enemy code can be exercised. The Galaga object
 * is shown commented out at the bottom, ready to be switched on.
 *
 * The grid is 8 columns by 5 rows to match the "5 waves of 8 ships" total of
 * 40 enemies. Waves themselves (entering a group at a time, then waiting for
 * them to be cleared) are part of Galaga, so all 40 are added at once here.
 *
 * @return 0 when the window is closed.
 */
//-----------------------------------------------------------------------
int main()
{
    // The window is 768 wide and 1024 tall; the player sits near the bottom.
    CMPUT350::GameEngine engine(768, 1024, "Galaga");
    
    auto player = std::make_shared<Player>(CMPUT350::Point2D(768 / 2, 900));
    engine.AddGameObject(player);
    engine.AddGameObject(std::make_shared<Stars>(250, CMPUT350::Rect(0, 0, 768, 1024)));

    // Lay the enemies out on a grid. Each enemy gets a Bezier entry path that
    // starts above the screen on alternating sides, so the formation fills in
    // from the left and right in a staggered pattern.
    for (int row = 0; row < 5; ++row)
    {
        for (int column = 0; column < 8; ++column)
        {
            const CMPUT350::Point2D formationLocation(
                84.0f + column * 85.0f,
                100.0f + row * 90.0f
            );
            const float side = (column + row) % 2 == 0 ? -150.0f : 150.0f;
            const std::vector<CMPUT350::Point2D> entryPath = {
                {formationLocation.x + side, -30.0f},
                {formationLocation.x + side, 100.0f},
                {formationLocation.x - side, formationLocation.y - 120.0f},
                formationLocation
            };

            // Cycle through the three enemy types across the grid so every
            // kind is present on screen.
            const int enemyType = (row * 8 + column) % 3;
            std::shared_ptr<Enemy> enemy;
            if (enemyType == 0)
            {
                enemy = std::make_shared<EnemyScout>(formationLocation, entryPath);
            }
            else if (enemyType == 1)
            {
                enemy = std::make_shared<EnemyWeaver>(formationLocation, entryPath);
            }
            else
            {
                enemy = std::make_shared<EnemyArmored>(formationLocation, entryPath);
            }
            engine.AddGameObject(enemy);
        }
    }

    // The Galaga class now exists on main and is meant to own the whole scene.
    // It is still being finished (its wave spawning is not done yet), so it is
    // left disabled here and the harness above keeps the scene playable. To
    // hand control over to it, delete the manual player/stars/enemy setup above
    // and enable these lines:
    //
    // auto game = std::make_shared<Galaga>(768, 1024);
    // engine.AddGameObject(game);

    engine.Run();

    return 0;
}
