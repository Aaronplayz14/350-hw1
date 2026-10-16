#ifndef ENGINEVIEW_H
#define ENGINEVIEW_H

#include <memory>
#include <string>
#include <vector>

namespace CMPUT350 {

class GameObject;

//-----------------------------------------------------------------------
/**
 * @brief The slice of the engine that game objects are allowed to see.
 *
 * Objects receive an EngineView through their GameContext, which lets them add
 * new objects, register key bindings, and iterate over the objects currently
 * in the scene without depending on the concrete GameEngine. The iteration
 * methods simply expose the engine's internal object vector.
 *
 * Key bindings are registered here rather than handled directly: a caller maps
 * a physical key to a notification name, and the engine turns key presses and
 * releases into those notifications. Several keys may map to the same name.
 */
//-----------------------------------------------------------------------
class EngineView {
public:
    using tGameObject = std::vector<std::shared_ptr<GameObject>>;
    using const_iterator = tGameObject::const_iterator;

    virtual ~EngineView() = default;

    /// @brief Queues an object to be added to the scene on the next frame.
    virtual void AddGameObject(std::shared_ptr<GameObject> gameObject) = 0;

    /// @brief Maps a key scancode to a notification sent when it is pressed.
    virtual void InstallKeyDownNotification(int key, const std::string& notification) = 0;

    /// @brief Maps a key scancode to a notification sent when it is released.
    virtual void InstallKeyUpNotification(int key, const std::string& notification) = 0;

    /// @brief Const iterator to the first live game object.
    virtual const_iterator cbegin() const = 0;
    /// @brief Const iterator one past the last live game object.
    virtual const_iterator cend() const = 0;
    /// @brief Const iterator to the first live game object.
    virtual const_iterator begin() const = 0;
    /// @brief Const iterator one past the last live game object.
    virtual const_iterator end() const = 0;
};

}  // namespace CMPUT350

#endif
