#include "GameEngine.h"

#include <iostream>

#include "DrawContext.h"
#include "FontData.h"
#include "GraphicsObject.h"
#include "CollisionObject.h"

namespace CMPUT350 {
namespace {

// The engine only cares about left/right, so arrow keys collapse onto the
// same chars the game already used. '\0' means "not a movement key".
char GetMovementKey(sf::Keyboard::Key key)
{
    switch (key)
    {
        case sf::Keyboard::Key::A:
        case sf::Keyboard::Key::Left:
            return 'a';
        case sf::Keyboard::Key::D:
        case sf::Keyboard::Key::Right:
            return 'd';
        default:
            return '\0';
    }
}

}  // namespace

GameEngine::GameEngine(unsigned int width, unsigned int height, const std::string& name)
{
    mWindow = std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), name);

    // Key repeat would fire HandleKeyState every few ms instead of once.
    mWindow->setKeyRepeatEnabled(false);

    // The assignment caps the engine at 30 fps.
    mWindow->setFramerateLimit(30);

    // Load the embedded font and give it to the drawing context.
    mFont = std::make_shared<sf::Font>();
    if (!mFont->openFromMemory(_font, _font_len))
        std::cerr << "WARNING: Font did not load.\n";

    mDrawContext = std::make_shared<DrawContext>(mWindow, mFont);

    // Point game objects back at the engine they belong to.
    mGameContext.EngineContext = this;
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
            if (event->is<sf::Event::Closed>())
            {
                mWindow->close();
                break;
            }

            // Held keys go through HandleKeyState so objects can poll them;
            // HandleKeyEvent stays for one-shot presses like firing.
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                const char movementKey = GetMovementKey(keyPressed->code);
                if (movementKey != '\0')
                {
                    for (auto& object : mGameObjects)
                        object->HandleKeyState(&mGameContext, movementKey, true);
                }
            }
            else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
            {
                const char movementKey = GetMovementKey(keyReleased->code);
                if (movementKey != '\0')
                {
                    for (auto& object : mGameObjects)
                        object->HandleKeyState(&mGameContext, movementKey, false);
                }
            }
            else if (const auto* text = event->getIf<sf::Event::TextEntered>())
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
        for (std::size_t i = 0; i + 1 < mGameObjects.size(); ++i)
        {
            std::shared_ptr<CollisionObject> objI = std::dynamic_pointer_cast<CollisionObject>(mGameObjects[i]);
            if (objI == nullptr)
                continue;

            for (std::size_t j = i + 1; j < mGameObjects.size(); ++j)
            {
                std::shared_ptr<CollisionObject> objIJ = std::dynamic_pointer_cast<CollisionObject>(mGameObjects[j]);
                if (objIJ == nullptr)
                    continue;

                CMPUT350::Rect boundsI = objI->GetBounds();
                CMPUT350::Rect boundsIJ = objIJ->GetBounds();

                // Rect::operator&= leaves width as NaN when the boxes miss,
                // so a real number here means they overlap.
                boundsI &= boundsIJ;
                if (!std::isnan(boundsI.width))
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