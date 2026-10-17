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
    /// @brief Opens the window and prepares the engine; width/height in pixels.
    GameEngine(unsigned int width, unsigned int height, const std::string& name);
    /// @brief Closes the window if it is still open.
    ~GameEngine();

    GameEngine(const GameEngine&) = delete;             // Prevent copy-construction
    GameEngine(GameEngine&&) = delete;                  // Prevent move-construction
    GameEngine& operator=(const GameEngine&) = delete;  // Prevent assignment
    GameEngine& operator=(GameEngine&&) = delete;       // Prevent move-assignment

    /// @brief Queues an object to be added at the start of the next frame.
    void AddGameObject(std::shared_ptr<GameObject> gameObject) override;
    /// @brief Registers a notification name to send when a key is pressed.
    void InstallKeyDownNotification(int key, const std::string& notification) override;

    /// @brief Registers a notification name to send when a key is released.
    void InstallKeyUpNotification(int key, const std::string& notification) override;

    /// @brief Removes every registered key press/release binding.
    void ClearKeyNotifications() override;

    /// @brief Const iterator to the first live object.
    const_iterator begin() const override;
    /// @brief Const iterator one past the last live object.
    const_iterator end() const override;

    /// @brief Const iterator to the first live object.
    const_iterator cbegin() const override;
    /// @brief Const iterator one past the last live object.
    const_iterator cend() const override;

    /// @brief Runs the main loop until the window closes.
    void Run();

private:
    using NotificationMap = std::unordered_map<int, std::vector<std::string>>;
    std::shared_ptr<sf::RenderWindow> mWindow;
    std::shared_ptr<sf::Font> mFont;
    std::shared_ptr<DrawContext> mDrawContext;
    std::shared_ptr<DrawContext> mGUIContext;

    // All objects active in the game, and any that were queued during a frame.
    tGameObject mGameObjects;
    tGameObject mObjectsToAdd;
    NotificationMap mKeyDownNotifications;
    NotificationMap mKeyUpNotifications;

    // Context handed to every game object.
    GameContext mGameContext;
    NotificationManager mNotificationManager;
};

}  // namespace CMPUT350

#endif  // GAMEENGINE_H