#include "GameEngine.h"

#include <iostream>

#include "DrawContext.h"
#include "FontData.h"
#include "GraphicsObject.h"
#include "CollisionObject.h"

namespace CMPUT350 {

GameEngine::GameEngine(unsigned int width, unsigned int height, const std::string& name)
{
    mWindow = std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), name);

    // The assignment caps the engine at 30 fps.
    mWindow->setFramerateLimit(30);

    // Load the embedded font and give it to the drawing context.
    mFont = std::make_shared<sf::Font>();
    if (!mFont->openFromMemory(_font, _font_len))
        std::cerr << "WARNING: Font did not load.\n";

    mDrawContext = std::make_shared<DrawContext>(mWindow, mFont);

    // Point game objects back at the engine they belong to.
    mGameContext.mEngineView = this;
    mGameContext.ScreenContext = mDrawContext.get();
}

GameEngine::~GameEngine()
{
    if (mWindow != nullptr && mWindow->isOpen())
        mWindow->close();
}

void GameEngine::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    mObjectsToAdd.push_back(gameObject);
}

void GameEngine::Run()
{
    while (mWindow->isOpen())
    {
        // 0. Remove dead objects. Swap with the last one so nothing is shifted.
        for (std::size_t i = 0; i < mGameObjects.size();)
        {
            if (!mGameObjects[i]->IsAlive())
            {
                mGameObjects[i] = mGameObjects.back();
                mGameObjects.pop_back();
            }
            else
            {
                ++i;
            }
        }

        // 1. Activate everything queued during the last frame.  Indexed loop
        //    because an object may add more objects while it initializes.
        for (std::size_t i = 0; i < mObjectsToAdd.size(); ++i)
        {
            mGameObjects.push_back(mObjectsToAdd[i]);
            mObjectsToAdd[i]->Initialize(&mGameContext);
        }
        mObjectsToAdd.clear();

        // 2. Window and keyboard events.
        while (const auto event = mWindow->pollEvent())
        {
            // Window close button.
            if (event->is<sf::Event::Closed>())
            {
                mWindow->close();
                break;
            }

            // We only care about the low-order ASCII characters.
            if (const auto* text = event->getIf<sf::Event::TextEntered>())
            {
                if (text->unicode <= 127)
                {
                    for (auto& object : mGameObjects)
                        object->HandleKeyEvent(&mGameContext, static_cast<char>(text->unicode));
                }
            }
        }
        if (!mWindow->isOpen())
            break;

        // 3. Simulate the game.
        for (auto& object : mGameObjects)
            object->Update(&mGameContext);

        // 4. Detect collisions.
        for (std::size_t i = 0; i < mGameObjects.size() - 1; ++i)
        {
            std::shared_ptr<CollisionObject> objI = std::dynamic_pointer_cast<CollisionObject>(mGameObjects[i]);
            if (objI == nullptr)
                continue;

            for (std::size_t j = 1; i + j < mGameObjects.size(); ++j)
            {
                std::shared_ptr<CollisionObject> objIJ = std::dynamic_pointer_cast<CollisionObject>(mGameObjects[i + j]);
                if (objIJ == nullptr)
                    continue;

                static CMPUT350::Rect boundsI({0, 0}, 0, 0);
                static CMPUT350::Rect boundsIJ({0, 0}, 0, 0);
                boundsI = objI->GetBounds();
                boundsIJ = objIJ->GetBounds();

                boundsI &= boundsIJ;
                if(!std::isnan(boundsI.width))
                {
                    objI->CollisionEnter(objIJ);
                    objIJ->CollisionEnter(objI);
                }
            }
        }

        // 5. Post-collision updates.
        for (auto& object : mGameObjects)
            object->LateUpdate(&mGameContext);

        // 6. Draw the scene, background first.
        mWindow->clear(sf::Color::Black);
        for (auto& object : mGameObjects)
        {
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get()))
                graphics->RenderBackground(&mGameContext);
        }

        // 7. Foreground on top.
        for (auto& object : mGameObjects)
        {
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get()))
                graphics->RenderForeground(&mGameContext);
        }

        mWindow->display();
    }
}

}  // namespace CMPUT350