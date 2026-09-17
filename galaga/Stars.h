#ifndef STARS_H
#define STARS_H

#include "GraphicsObject.h"
#include <random>

class Stars : public CMPUT350::GraphicsObject
{
public:
    Stars(int numStars, CMPUT350::Rect bounds);
    void RenderBackground(CMPUT350::GameContext* context) override;

private:
    std::vector<CMPUT350::Point2D> mStarPositions;
    CMPUT350::Rect mBounds;
    std::random_device rd;
    std::mt19937 gen;
};


#endif // STARS_H
