#ifndef PLAYER_H
#define PLAYER_H

#include <string>

#include "CollisionObject.h"
#include "Bullet.h"

class Player : public CMPUT350::CollisionObject
{
public:
    Player(CMPUT350::Point2D loc);

    // GameObject Functions
    void Initialize(CMPUT350::GameContext* context) override;
    void Update(CMPUT350::GameContext* context) override;
    void LateUpdate(CMPUT350::GameContext* context) override;
    void ReceiveNotification(const std::string& notification) override;

    bool IsAlive() const override;
    void Kill() override;

    // Graphics Object Functions
    void RenderBackground(CMPUT350::GameContext* context) override;
    void RenderForeground(CMPUT350::GameContext* context) override;
    CMPUT350::Point2D GetLocation() const override;
    float GetRotation() const override;

    // Collision Object Functions
    void CollisionEnter(const std::shared_ptr<CMPUT350::CollisionObject>& obj) override;
    const CMPUT350::Rect& GetBounds() override;
    const std::vector<CMPUT350::Shape>& GetShapes() override;

private:
    CMPUT350::Point2D mLoc;
    CMPUT350::Rect mBounds;
    bool mAlive;
    float mSpeed;
    bool mMoveLeft;
    bool mMoveRight;
    bool mFireRequested;
    std::vector<CMPUT350::Shape> mShapes;

    // Weak on purpose: the engine owns the bullets, and an expired slot is
    // what tells us we are allowed to fire again.
    std::weak_ptr<Bullet> b1, b2;
};

#endif
