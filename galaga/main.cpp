#include "GameEngine.h"
#include "Galaga.h"

//-----------------------------------------------------------------------
/**
 * @brief Entry point.
 *
 * The program does nothing but create the engine and hand the whole scene to a
 * single Galaga object. Galaga owns the player, the enemies, the score, and the
 * game-state machine, and creates/removes everything else as it runs.
 *
 * @return 0 when the window is closed.
 */
//-----------------------------------------------------------------------
int main()
{
    CMPUT350::GameEngine engine(768, 1024, "Galaga");
    auto game = std::make_shared<Galaga>(768, 1024);
    engine.AddGameObject(game);

    engine.Run();

    return 0;
}
