#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>

namespace CMPUT350 {

class GameContext;

class GameObject {
public:
    virtual ~GameObject() = default;
    virtual void Initialize(GameContext *context);
    virtual void Update(GameContext *context);
    virtual void LateUpdate(GameContext *context);
    virtual void RenderUI(GameContext *context);

    virtual void ReceiveNotification(const std::string& notification);

    virtual bool IsAlive() const;
    virtual void Kill();
};

}  // namespace CMPUT350

#endif  // GAMEOBJECT_H
