#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <memory>
#include <string>
#include <vector>

#include "EngineView.h"
#include "GameContext.h"
#include "GameObject.h"

#include <SFML/Graphics.hpp>

namespace CMPUT350 {

class GameEngine : public EngineView {
public:
    GameEngine(unsigned int width, unsigned int height, const std::string& name);
    ~GameEngine();

    GameEngine(const GameEngine&) = delete;             // Prevent copy-construction
    GameEngine(GameEngine&&) = delete;                  // Prevent move-construction
    GameEngine& operator=(const GameEngine&) = delete;  // Prevent assignment
    GameEngine& operator=(GameEngine&&) = delete;       // Prevent move-assignment

    void AddGameObject(std::shared_ptr<GameObject> gameObject) override;

    void Run();

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    std::shared_ptr<sf::Font> mFont;
    std::shared_ptr<DrawContext> mDrawContext;

    // All objects active in the game, and any that were queued during a frame.
    std::vector<std::shared_ptr<GameObject>> mGameObjects;
    std::vector<std::shared_ptr<GameObject>> mObjectsToAdd;

    // Context handed to every game object.
    GameContext mGameContext;
};

}  // namespace CMPUT350

#endif  // GAMEENGINE_H