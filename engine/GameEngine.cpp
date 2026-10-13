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
                Rect boundsI = objI->GetBounds();
                Rect boundsIJ = objIJ->GetBounds();

                // Rect::operator&= leaves width as NaN when the boxes miss,
                // so a real number here means they overlap.
                boundsI &= boundsIJ;
                if (!std::isnan(boundsI.width))
                {
                    std::vector<Shape> shapesI = objI->GetShapes();
                    std::vector<Shape> shapesIJ = objIJ->GetShapes();

                    for (std::size_t u; u < shapesI.size(); ++u)
                    {
                        for (std::size_t v; v < shapesIJ.size(); ++v)
                        {
                            // Rect-Rect Collision
                            if(shapesI[u].t == ShapeType::kRect && shapesIJ[v].t == ShapeType::kRect) {
                                Rect rect1 = shapesI[u].shape.rect + objI->GetLocation();
                                Rect rect2 = shapesIJ[v].shape.rect + objIJ->GetLocation();
                                rect1 &= rect2;
                                if (!std::isnan(rect1.width)) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }

                            // Circle-Circle Collision    
                            } else if (shapesI[u].t == ShapeType::kCircle && shapesIJ[v].t == ShapeType::kCircle) {
                                Point2D center1 = shapesI[u].shape.circle.center + objI->GetLocation();
                                Point2D center2 = shapesIJ[v].shape.circle.center + objIJ->GetLocation();
                                if (center1.Distance(center2) <= shapesI[u].shape.circle.radius + shapesIJ[v].shape.circle.radius) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }

                            // Line-Line Collision
                            } else if (shapesI[u].t == ShapeType::kLine && shapesIJ[v].t == ShapeType::kLine) {
                                Point2D crossPoint = Point2D();
                                Line line1 = Line(shapesI[u].shape.line.p1 + objI->GetLocation(), shapesI[u].shape.line.p2 + objI->GetLocation());
                                Line line2 = Line(shapesIJ[v].shape.line.p1 + objIJ->GetLocation(), shapesIJ[v].shape.line.p2 + objIJ->GetLocation());
                                if (line1.Crosses(line2, crossPoint)) {
                                    objI->CollisionEnter(objIJ, crossPoint);
                                    objIJ->CollisionEnter(objI, crossPoint);
                                }

                            // Circle-Line Collision
                            } else if (shapesI[u].t == ShapeType::kCircle && shapesIJ[v].t == ShapeType::kLine) {
                                Circle circ1 = Circle(shapesI[u].shape.circle.center + objI->GetLocation(), shapesI[u].shape.circle.radius);
                                Line line1 = Line(shapesIJ[v].shape.line.p1 + objIJ->GetLocation(), shapesIJ[v].shape.line.p2 + objIJ->GetLocation());
                                Point2D closePoint = line1.ClosestPoint(circ1.center);
                                if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                    float y = circ1.center.Distance(closePoint);
                                    float x = sqrtf(circ1.radius * circ1.radius - y * y);
                                    Point2D collisionPoint = closePoint + ((line1.p2 - closePoint) * (x / line1.p2.Distance(closePoint)));
                                    objI->CollisionEnter(objIJ, collisionPoint);
                                    objIJ->CollisionEnter(objI, collisionPoint);
                                }

                            // Line-Circle Collision
                            } else if (shapesI[u].t == ShapeType::kLine && shapesIJ[v].t == ShapeType::kCircle) {
                                Circle circ1 = Circle(shapesIJ[v].shape.circle.center + objIJ->GetLocation(), shapesIJ[v].shape.circle.radius);
                                Line line1 = Line(shapesI[u].shape.line.p1 + objI->GetLocation(), shapesI[u].shape.line.p2 + objI->GetLocation());
                                Point2D closePoint = line1.ClosestPoint(circ1.center);
                                if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                    float y = circ1.center.Distance(closePoint);
                                    float x = sqrtf(circ1.radius * circ1.radius - y * y);
                                    Point2D collisionPoint = closePoint + ((line1.p2 - closePoint) * (x / line1.p2.Distance(closePoint)));
                                    objI->CollisionEnter(objIJ, collisionPoint);
                                    objIJ->CollisionEnter(objI, collisionPoint);
                                }

                            // Circle-Rect Collision
                            } else if (shapesI[u].t == ShapeType::kCircle && shapesIJ[v].t == ShapeType::kRect) {
                                Circle circ1 = Circle(shapesI[u].shape.circle.center + objI->GetLocation(), shapesI[u].shape.circle.radius);
                                Rect rect1 = shapesIJ[v].shape.rect + objIJ->GetLocation();
                                Line rectLine1 = Line(rect1.topLeft, rect1.topLeft + Point2D(rect1.width, 0.0));
                                Line rectLine2 = Line(rect1.topLeft, rect1.topLeft + Point2D(0.0, rect1.height)); 
                                Line rectLine3 = Line(rect1.topLeft + Point2D(rect1.width, 0.0), rect1.topLeft + Point2D(rect1.width, rect1.height)); 
                                Line rectLine4 = Line(rect1.topLeft + Point2D(0.0, rect1.height), rect1.topLeft + Point2D(rect1.width, rect1.height));
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                for (std::size_t w; w < rectLines.size(); ++w) {
                                    Point2D closePoint = rectLines[w].ClosestPoint(circ1.center);
                                    if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                        objI->CollisionEnter(objIJ);
                                        objIJ->CollisionEnter(objI);
                                        continue;
                                    }
                                }

                            } else if (shapesI[u].t == ShapeType::kRect && shapesIJ[v].t == ShapeType::kCircle) {
                                Circle circ1 = Circle(shapesIJ[v].shape.circle.center + objIJ->GetLocation(), shapesIJ[v].shape.circle.radius);
                                Rect rect1 = shapesI[u].shape.rect + objI->GetLocation();
                                Line rectLine1 = Line(rect1.topLeft, rect1.topLeft + Point2D(rect1.width, 0.0));
                                Line rectLine2 = Line(rect1.topLeft, rect1.topLeft + Point2D(0.0, rect1.height)); 
                                Line rectLine3 = Line(rect1.topLeft + Point2D(rect1.width, 0.0), rect1.topLeft + Point2D(rect1.width, rect1.height)); 
                                Line rectLine4 = Line(rect1.topLeft + Point2D(0.0, rect1.height), rect1.topLeft + Point2D(rect1.width, rect1.height));
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                for (std::size_t w; w < rectLines.size(); ++w) {
                                    Point2D closePoint = rectLines[w].ClosestPoint(circ1.center);
                                    if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                        objI->CollisionEnter(objIJ);
                                        objIJ->CollisionEnter(objI);
                                        continue;
                                    }
                                }

                            // Line-Rect Collision
                            } else if (shapesI[u].t == ShapeType::kLine && shapesIJ[v].t == ShapeType::kRect) {
                                Line line1 = Line(shapesI[u].shape.line.p1 + objI->GetLocation(), shapesI[u].shape.line.p2 + objI->GetLocation());
                                Rect rect1 = shapesIJ[v].shape.rect + objIJ->GetLocation();
                                Line rectLine1 = Line(rect1.topLeft, rect1.topLeft + Point2D(rect1.width, 0.0));
                                Line rectLine2 = Line(rect1.topLeft, rect1.topLeft + Point2D(0.0, rect1.height)); 
                                Line rectLine3 = Line(rect1.topLeft + Point2D(rect1.width, 0.0), rect1.topLeft + Point2D(rect1.width, rect1.height)); 
                                Line rectLine4 = Line(rect1.topLeft + Point2D(0.0, rect1.height), rect1.topLeft + Point2D(rect1.width, rect1.height));
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                Point2D collisionPoint = Point2D();
                                float dist = INFINITY;
                                for (std::size_t w; w < rectLines.size(); ++w) {
                                    Point2D crossPoint = Point2D();
                                    if (line1.Crosses(rectLines[w], crossPoint)) {
                                        if (line1.p2.Distance(crossPoint) < dist) {
                                            collisionPoint = crossPoint;
                                            dist = line1.p2.Distance(crossPoint);
                                        }
                                    }
                                }
                                if (std::isfinite(dist)) {
                                    objI->CollisionEnter(objIJ, collisionPoint);
                                    objIJ->CollisionEnter(objI, collisionPoint);
                                }

                            // Rect-Line Collision
                            } else if (shapesI[u].t == ShapeType::kRect && shapesIJ[v].t == ShapeType::kLine) {
                                Line line1 = Line(shapesIJ[v].shape.line.p1 + objIJ->GetLocation(), shapesIJ[v].shape.line.p2 + objIJ->GetLocation());
                                Rect rect1 = shapesI[u].shape.rect + objI->GetLocation();
                                Line rectLine1 = Line(rect1.topLeft, rect1.topLeft + Point2D(rect1.width, 0.0));
                                Line rectLine2 = Line(rect1.topLeft, rect1.topLeft + Point2D(0.0, rect1.height)); 
                                Line rectLine3 = Line(rect1.topLeft + Point2D(rect1.width, 0.0), rect1.topLeft + Point2D(rect1.width, rect1.height)); 
                                Line rectLine4 = Line(rect1.topLeft + Point2D(0.0, rect1.height), rect1.topLeft + Point2D(rect1.width, rect1.height));
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                Point2D collisionPoint = Point2D();
                                float dist = INFINITY;
                                for (std::size_t w; w < rectLines.size(); ++w) {
                                    Point2D crossPoint = Point2D();
                                    if (line1.Crosses(rectLines[w], crossPoint)) {
                                        if (line1.p2.Distance(crossPoint) < dist) {
                                            collisionPoint = crossPoint;
                                            dist = line1.p2.Distance(crossPoint);
                                        }
                                    }
                                }
                                if (std::isfinite(dist)) {
                                    objI->CollisionEnter(objIJ, collisionPoint);
                                    objIJ->CollisionEnter(objI, collisionPoint);
                                }
                            }
                        }
                    }                    
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