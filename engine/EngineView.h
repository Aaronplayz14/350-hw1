#ifndef ENGINEVIEW_H
#define ENGINEVIEW_H

#include <memory>
#include <string>
#include <vector>

namespace CMPUT350 {

class GameObject;

class EngineView {
public:
    using ObjectList = std::vector<std::shared_ptr<GameObject>>;
    virtual ~EngineView() = default;
    virtual void AddGameObject(std::shared_ptr<GameObject> gameObject) = 0;
    virtual void InstallKeyDownNotification(int key, const std::string& notification) = 0;
    virtual void InstallKeyUpNotification(int key, const std::string& notification) = 0;

    virtual ObjectList::iterator begin() = 0;
    virtual ObjectList::iterator end() = 0;

    virtual ObjectList::const_iterator cbegin() const = 0;
    virtual ObjectList::const_iterator cend() const = 0;

};

}  // namespace CMPUT350

#endif
