#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "EngineView.h"
#include "GameContext.h"
#include "GameObject.h"
#include "NotificationManager.h"

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
    void InstallKeyDownNotification(int key, const std::string& notification) override;

    void InstallKeyUpNotification(int key, const std::string& notification) override;

    ObjectList::iterator begin() override;
    ObjectList::iterator end() override;

    ObjectList::const_iterator cbegin() const override;
    ObjectList::const_iterator cend() const override;
    void Run();

private:
using NotificationMap = std::unordered_map<int, std::vector<std::string>>;
    std::shared_ptr<sf::RenderWindow> mWindow;
    std::shared_ptr<sf::Font> mFont;
    std::shared_ptr<DrawContext> mDrawContext;
    std::shared_ptr<DrawContext> mGUIContext;

    // All objects active in the game, and any that were queued during a frame.
    std::vector<std::shared_ptr<GameObject>> mGameObjects;
    std::vector<std::shared_ptr<GameObject>> mObjectsToAdd;
    NotificationMap mKeyDownNotifications;
    NotificationMap mKeyUpNotifications;

    // Context handed to every game object.
    GameContext mGameContext;
    NotificationManager mNotificationManager;
};

}  // namespace CMPUT350

#endif  // GAMEENGINE_H