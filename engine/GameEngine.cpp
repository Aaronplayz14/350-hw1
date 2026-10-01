#include "GameEngine.h"

#include <iostream>

#include "DrawContext.h"
#include "FontData.h"
#include "GraphicsObject.h"
#include "CollisionObject.h"

namespace CMPUT350 {
namespace {

//-----------------------------------------------------------------------
/**
 * @brief Maps an SFML key code onto the movement characters the game expects.
 *
 * The engine only cares about left and right, so the arrow keys collapse onto
 * the same characters the game already used. '\0' means "not a movement key",
 * which lets the caller skip dispatching the event entirely.
 *
 * @param key The key code reported by a KeyPressed or KeyReleased event.
 * @return 'a' for left, 'd' for right, and '\0' for every other key.
 */
//-----------------------------------------------------------------------
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

//-----------------------------------------------------------------------
/**
 * @brief Opens the game window and prepares the engine for its main loop.
 *
 * Creates the render window, disables key repeat so held keys report once
 * rather than every few milliseconds, caps the frame rate, and loads the
 * embedded font into a DrawContext. The game objects are told about the
 * engine through mGameContext before Run is called.
 *
 * A font that fails to load is reported on stderr rather than aborting, so
 * the window still opens and the rest of the program stays usable.
 *
 * @param width Width of the window in pixels.
 * @param height Height of the window in pixels.
 * @param name Title shown in the window's title bar.
 */
//-----------------------------------------------------------------------
GameEngine::GameEngine(unsigned int width, unsigned int height, const std::string& name)
{
    mWindow = std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), name);

    // Key repeat would fire HandleKeyState every few ms instead of once.
    mWindow->setKeyRepeatEnabled(false);

    // The assignment caps the engine at 30 fps.
    mWindow->setFramerateLimit(30);

    // Load the embedded font and give it to the drawing context.
    mFont = std::make_shared<sf::Font>();
    // Warn but continue: the window is still usable without the font.
    if (!mFont->openFromMemory(_font, _font_len))
        std::cerr << "WARNING: Font did not load.\n";

    mDrawContext = std::make_shared<DrawContext>(mWindow, mFont);

    // Point game objects back at the engine they belong to. The drawing
    // context is a raw pointer because DrawContext is owned above and
    // outlives every object that borrows it.
    mGameContext.EngineContext = this;
    mGameContext.ScreenContext = mDrawContext.get();
}

//-----------------------------------------------------------------------
/**
 * @brief Closes the window if it is still open.
 *
 * SFML tears the window down on its own, but closing it explicitly releases
 * the drawing surface before the shared pointers are released.
 */
//-----------------------------------------------------------------------
GameEngine::~GameEngine()
{
    if (mWindow != nullptr && mWindow->isOpen())
        mWindow->close();
}

//-----------------------------------------------------------------------
/**
 * @brief Queues a game object to be added at the start of the next frame.
 *
 * The object is not stored immediately. Queueing keeps the object list stable
 * while it is being iterated, so an object is free to spawn more objects
 * during Initialize or Update without invalidating any live iterator.
 *
 * @param gameObject The object to own; held as a shared_ptr and kept alive
 *                   until it reports itself dead.
 * @return None. The object appears in the scene on the next frame.
 */
//-----------------------------------------------------------------------
void GameEngine::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    mObjectsToAdd.push_back(gameObject);
}

//-----------------------------------------------------------------------
/**
 * @brief Runs the main game loop until the window is closed.
 *
 * Each frame runs a fixed sequence of passes. The order matters: dead objects
 * are removed first so nothing is simulated or drawn after it dies, newly
 * queued objects are activated before updates so they take part immediately,
 * collisions are detected after movement so they reflect the current frame's
 * positions, and background rendering is separated from foreground rendering
 * so stars and similar layers sit behind the ships.
 *
 * Collision detection is a pairwise sweep over objects deriving from
 * CollisionObject. Only the upper triangle is visited (j starts at i + 1),
 * so each pair is tested once per frame and both objects are notified.
 *
 * @return None. Blocks until the window is closed or a Closed event arrives.
 */
//-----------------------------------------------------------------------
void GameEngine::Run()
{
    while (mWindow->isOpen())
    {
        // 0. Remove dead objects. Swap with the last one so nothing is
        //    shifted; the loop deliberately does not advance i after a swap,
        //    because the element moved into position i is unchecked.
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
        // Cleared only after the loop: Initialize above may have queued more.
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

        // 4. Detect collisions. The outer bound stops one short of the end and
        //    the inner loop starts at i + 1, so each pair is visited exactly
        //    once and no object is compared against itself.
        for (std::size_t i = 0; i + 1 < mGameObjects.size(); ++i)
        {
            // Objects not deriving from CollisionObject cannot collide.
            std::shared_ptr<CollisionObject> objI = std::dynamic_pointer_cast<CollisionObject>(mGameObjects[i]);
            if (objI == nullptr)
                continue;

            for (std::size_t j = i + 1; j < mGameObjects.size(); ++j)
            {
                std::shared_ptr<CollisionObject> objIJ = std::dynamic_pointer_cast<CollisionObject>(mGameObjects[j]);
                if (objIJ == nullptr)
                    continue;

                // Copy the bounds: operator&= mutates in place and we must
                // not disturb the object's own stored rectangle.
                CMPUT350::Rect boundsI = objI->GetBounds();
                CMPUT350::Rect boundsIJ = objIJ->GetBounds();

                // Rect::operator&= leaves width as NaN when the boxes miss,
                // so a real number here means they overlap.
                boundsI &= boundsIJ;
                if (!std::isnan(boundsI.width))
                {
                    // Both sides are notified, so either object may react.
                    objI->CollisionEnter(objIJ);
                    objIJ->CollisionEnter(objI);
                }
            }
        }

        // 5. Post-collision updates.
        for (auto& object : mGameObjects)
            object->LateUpdate(&mGameContext);

        // 6. Draw the scene, background first. Two full passes rather
        //    than one combined pass, because everything in the first group
        //    must land underneath everything in the second (stars behind ships).
        mWindow->clear(sf::Color::Black);
        for (auto& object : mGameObjects)
        {
            // raw pointer here: the shared_ptr must not gain an owner.
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get()))
                graphics->RenderBackground(&mGameContext);
        }

        // 7. Foreground on top. Separate pass so ordering against the
        //    background group is guaranteed by draw order, not object order.
        for (auto& object : mGameObjects)
        {
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get()))
                graphics->RenderForeground(&mGameContext);
        }

        mWindow->display();
    }
}

}  // namespace CMPUT350