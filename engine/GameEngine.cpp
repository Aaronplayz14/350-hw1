#include "GameEngine.h"

#include <iostream>
#include <algorithm>

#include "DrawContext.h"
#include "FontData.h"
#include "GraphicsObject.h"
#include "CollisionObject.h"

namespace CMPUT350 {
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

    // Key repeat would send repeated key-down notifications instead of one.
    mWindow->setKeyRepeatEnabled(false);

    // The assignment caps the engine at 30 fps.
    mWindow->setFramerateLimit(30);

    // Load the embedded font and give it to the drawing context.
    mFont = std::make_shared<sf::Font>();
    // Warn but continue: the window is still usable without the font.
    if (!mFont->openFromMemory(_font, _font_len))
        std::cerr << "WARNING: Font did not load.\n";

    mDrawContext = std::make_shared<DrawContext>(mWindow, mFont);
    mGUIContext = std::make_shared<DrawContext>(mWindow, mFont);

    // Fill in the context every object will receive. The engine view and the
    // notification manager are pointed back at this engine, and the two draw
    // contexts are borrowed as raw pointers because they are owned above and
    // outlive every object that uses them.
    mGameContext.mEngineView = this;
    mGameContext.ScreenContext = mDrawContext.get();
    mGameContext.GUIContext = mGUIContext.get();
    mGameContext.mNotificationManager = &mNotificationManager;
    mGameContext.currentFrame = 0;
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
 * @brief Registers a notification to send when a key is pressed.
 *
 * The same key can map to several notifications (for example both A and Left
 * map to a "move left" action), and a duplicate mapping is ignored.
 * @param key Scancode to watch, from CMPUT350::Key.
 * @param notification Notification name to send when the key goes down.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameEngine::InstallKeyDownNotification(int key, const std::string& notification)
{
    if (notification.empty()) {
        return;
    }

    auto& notifications = mKeyDownNotifications[key];

    // The same key/action pair should be stored only once.
    if (std::find(
            notifications.begin(),
            notifications.end(),
            notification
        ) == notifications.end()) {
        notifications.push_back(notification);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Registers a notification to send when a key is released.
 * @param key Scancode to watch, from CMPUT350::Key.
 * @param notification Notification name to send when the key comes up.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameEngine::InstallKeyUpNotification(
    int key,
    const std::string& notification
)
{
    if (notification.empty()) {
        return;
    }

    auto& notifications = mKeyUpNotifications[key];

    if (std::find(
            notifications.begin(),
            notifications.end(),
            notification
        ) == notifications.end()) {
        notifications.push_back(notification);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Const iterator to the first live game object.
 * @return begin() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::begin() const
{
    return mGameObjects.begin();
}

//-----------------------------------------------------------------------
/**
 * @brief Const iterator one past the last live game object.
 * @return end() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::end() const
{
    return mGameObjects.end();
}

//-----------------------------------------------------------------------
/**
 * @brief Const iterator to the first live game object.
 * @return cbegin() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::cbegin() const
{
    return mGameObjects.cbegin();
}

//-----------------------------------------------------------------------
/**
 * @brief Const iterator one past the last live game object.
 * @return cend() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::cend() const
{
    return mGameObjects.cend();
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
 * so stars and similar layers sit behind the ships. A final UI pass lets each
 * object draw untransformed HUD content on top.
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
            mGameContext.CurrObject = mObjectsToAdd[i];
            mObjectsToAdd[i]->Initialize(&mGameContext);
            mGameContext.CurrObject.reset();
        }
        // Cleared only after the loop: Initialize above may have queued more.
        mObjectsToAdd.clear();

        mNotificationManager.AdvanceFrame(mGameContext.currentFrame);

        // 2. Window and keyboard events.
        while (const auto event = mWindow->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                mWindow->close();
                break;
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                const auto found = mKeyDownNotifications.find(
                    static_cast<int>(keyPressed->scancode)
                );
                if (found != mKeyDownNotifications.end()) {
                    for (const auto& notification : found->second) {
                        mNotificationManager.Notify(notification);
                    }
                }
            }
            else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
            {
                const auto found = mKeyUpNotifications.find(
                    static_cast<int>(keyReleased->scancode)
                );
                if (found != mKeyUpNotifications.end()) {
                    for (const auto& notification : found->second) {
                        mNotificationManager.Notify(notification);
                    }
                }
            }
        }
        if (!mWindow->isOpen())
            break;

        // 3. Simulate the game.
        for (auto& object : mGameObjects)
        {
            mGameContext.CurrObject = object;
            object->Update(&mGameContext);
            mGameContext.CurrObject.reset();
        }

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

                    for (std::size_t u = 0; u < shapesI.size(); ++u)
                    {
                        for (std::size_t v = 0; v < shapesIJ.size(); ++v)
                        {
                            // Rect-Rect Collision
                            if(shapesI[u].t == ShapeType::kRect && shapesIJ[v].t == ShapeType::kRect) {
                                // Rect 1
                                std::vector<Point2D> rect1Points = shapesI[u].shape.rect.Points();
                                Line rect1Line1 = Line(rect1Points[0].Rotate(objI->GetRotation()) + objI->GetLocation(), rect1Points[1].Rotate(objI->GetRotation()) + objI->GetLocation());
                                Line rect1Line2 = Line(rect1Points[0].Rotate(objI->GetRotation()) + objI->GetLocation(), rect1Points[2].Rotate(objI->GetRotation()) + objI->GetLocation()); 
                                Line rect1Line3 = Line(rect1Points[1].Rotate(objI->GetRotation()) + objI->GetLocation(), rect1Points[3].Rotate(objI->GetRotation()) + objI->GetLocation()); 
                                Line rect1Line4 = Line(rect1Points[2].Rotate(objI->GetRotation()) + objI->GetLocation(), rect1Points[3].Rotate(objI->GetRotation()) + objI->GetLocation());
                                std::vector<Line> rect1Lines = {rect1Line1, rect1Line2, rect1Line3, rect1Line4};
                                // Rect 2
                                std::vector<Point2D> rect2Points = shapesIJ[v].shape.rect.Points();
                                Line rect2Line1 = Line(rect2Points[0].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rect2Points[1].Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                Line rect2Line2 = Line(rect2Points[0].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rect2Points[2].Rotate(objIJ->GetRotation()) + objIJ->GetLocation()); 
                                Line rect2Line3 = Line(rect2Points[1].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rect2Points[3].Rotate(objIJ->GetRotation()) + objIJ->GetLocation()); 
                                Line rect2Line4 = Line(rect2Points[2].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rect2Points[3].Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                std::vector<Line> rect2Lines = {rect2Line1, rect2Line2, rect2Line3, rect2Line4};
                                
                                bool cross = false;
                                Point2D crossPoint = Point2D();
                                for (std::size_t m = 0; (m < rect1Lines.size()) && !cross; ++m) {
                                    for (std::size_t n = 0; (n < rect2Lines.size()) && !cross; ++n) {
                                        if (rect1Lines[m].Crosses(rect2Lines[n], crossPoint)) {
                                            Point2D cPointI = crossPoint - objI->GetLocation();
                                            Point2D cPointIJ = crossPoint - objIJ->GetLocation();
                                            objI->CollisionEnter(objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                            objIJ->CollisionEnter(objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                            cross = true;
                                        }
                                    }
                                }
                            // Circle-Circle Collision    
                            } else if (shapesI[u].t == ShapeType::kCircle && shapesIJ[v].t == ShapeType::kCircle) {
                                Point2D center1 = shapesI[u].shape.circle.center.Rotate(objI->GetRotation()) + objI->GetLocation();
                                Point2D center2 = shapesIJ[v].shape.circle.center.Rotate(objIJ->GetRotation()) + objIJ->GetLocation();
                                if (center1.Distance(center2) <= shapesI[u].shape.circle.radius + shapesIJ[v].shape.circle.radius) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }

                            // Line-Line Collision
                            } else if (shapesI[u].t == ShapeType::kLine && shapesIJ[v].t == ShapeType::kLine) {
                                Point2D crossPoint = Point2D();
                                Line line1 = Line(shapesI[u].shape.line.p1.Rotate(objI->GetRotation()) + objI->GetLocation(), 
                                                  shapesI[u].shape.line.p2.Rotate(objI->GetRotation()) + objI->GetLocation());
                                Line line2 = Line(shapesIJ[v].shape.line.p1.Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), 
                                                  shapesIJ[v].shape.line.p2.Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                if (line1.Crosses(line2, crossPoint)) {
                                    Point2D cPointI = crossPoint - objI->GetLocation();
                                    Point2D cPointIJ = crossPoint - objIJ->GetLocation();
                                    objI->CollisionEnter(objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                    objIJ->CollisionEnter(objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                }

                            // Circle-Line Collision
                            } else if (shapesI[u].t == ShapeType::kCircle && shapesIJ[v].t == ShapeType::kLine) {
                                Circle circ1 = Circle(shapesI[u].shape.circle.center.Rotate(objI->GetRotation()) + objI->GetLocation(), shapesI[u].shape.circle.radius);
                                Line line1 = Line(shapesIJ[v].shape.line.p1.Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), 
                                                  shapesIJ[v].shape.line.p2.Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                Point2D closePoint = line1.ClosestPoint(circ1.center);
                                if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                    float y = circ1.center.Distance(closePoint);
                                    float x = sqrtf(circ1.radius * circ1.radius - y * y);
                                    Point2D collisionPoint = closePoint + ((line1.p2 - closePoint) * (x / line1.p2.Distance(closePoint)));
                                    Point2D cPointI = collisionPoint - objI->GetLocation();
                                    Point2D cPointIJ = collisionPoint - objIJ->GetLocation();
                                    objI->CollisionEnter(objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                    objIJ->CollisionEnter(objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                }

                            // Line-Circle Collision
                            } else if (shapesI[u].t == ShapeType::kLine && shapesIJ[v].t == ShapeType::kCircle) {
                                Circle circ1 = Circle(shapesIJ[v].shape.circle.center.Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), shapesIJ[v].shape.circle.radius);
                                Line line1 = Line(shapesI[u].shape.line.p1.Rotate(objI->GetRotation()) + objI->GetLocation(), 
                                                  shapesI[u].shape.line.p2.Rotate(objI->GetRotation()) + objI->GetLocation());
                                Point2D closePoint = line1.ClosestPoint(circ1.center);
                                if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                    float y = circ1.center.Distance(closePoint);
                                    float x = sqrtf(circ1.radius * circ1.radius - y * y);
                                    Point2D collisionPoint = closePoint + ((line1.p2 - closePoint) * (x / line1.p2.Distance(closePoint)));
                                    Point2D cPointI = collisionPoint - objI->GetLocation();
                                    Point2D cPointIJ = collisionPoint - objIJ->GetLocation();
                                    objI->CollisionEnter(objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                    objIJ->CollisionEnter(objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                }

                            // Circle-Rect Collision
                            } else if (shapesI[u].t == ShapeType::kCircle && shapesIJ[v].t == ShapeType::kRect) {
                                Circle circ1 = Circle(shapesI[u].shape.circle.center.Rotate(objI->GetRotation()) + objI->GetLocation(), shapesI[u].shape.circle.radius);
                                std::vector<Point2D> rectPoints = shapesIJ[v].shape.rect.Points();
                                Line rectLine1 = Line(rectPoints[0].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[1].Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                Line rectLine2 = Line(rectPoints[0].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[2].Rotate(objIJ->GetRotation()) + objIJ->GetLocation()); 
                                Line rectLine3 = Line(rectPoints[1].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[3].Rotate(objIJ->GetRotation()) + objIJ->GetLocation()); 
                                Line rectLine4 = Line(rectPoints[2].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[3].Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                for (std::size_t w = 0; w < rectLines.size(); ++w) {
                                    Point2D closePoint = rectLines[w].ClosestPoint(circ1.center);
                                    if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                        objI->CollisionEnter(objIJ);
                                        objIJ->CollisionEnter(objI);
                                        continue;
                                    }
                                }

                            // Rect-Circle Collision
                            } else if (shapesI[u].t == ShapeType::kRect && shapesIJ[v].t == ShapeType::kCircle) {
                                Circle circ1 = Circle(shapesIJ[v].shape.circle.center.Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), shapesIJ[v].shape.circle.radius);
                                std::vector<Point2D> rectPoints = shapesI[u].shape.rect.Points();
                                Line rectLine1 = Line(rectPoints[0].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[1].Rotate(objI->GetRotation()) + objI->GetLocation());
                                Line rectLine2 = Line(rectPoints[0].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[2].Rotate(objI->GetRotation()) + objI->GetLocation()); 
                                Line rectLine3 = Line(rectPoints[1].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[3].Rotate(objI->GetRotation()) + objI->GetLocation()); 
                                Line rectLine4 = Line(rectPoints[2].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[3].Rotate(objI->GetRotation()) + objI->GetLocation());
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                for (std::size_t w = 0; w < rectLines.size(); ++w) {
                                    Point2D closePoint = rectLines[w].ClosestPoint(circ1.center);
                                    if (circ1.center.Distance(closePoint) <= circ1.radius) {
                                        objI->CollisionEnter(objIJ);
                                        objIJ->CollisionEnter(objI);
                                        continue;
                                    }
                                }

                            // Line-Rect Collision
                            } else if (shapesI[u].t == ShapeType::kLine && shapesIJ[v].t == ShapeType::kRect) {
                                Line line1 = Line(shapesI[u].shape.line.p1.Rotate(objI->GetRotation()) + objI->GetLocation(), 
                                                  shapesI[u].shape.line.p2.Rotate(objI->GetRotation()) + objI->GetLocation());
                                std::vector<Point2D> rectPoints = shapesIJ[v].shape.rect.Points();
                                Line rectLine1 = Line(rectPoints[0].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[1].Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                Line rectLine2 = Line(rectPoints[0].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[2].Rotate(objIJ->GetRotation()) + objIJ->GetLocation()); 
                                Line rectLine3 = Line(rectPoints[1].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[3].Rotate(objIJ->GetRotation()) + objIJ->GetLocation()); 
                                Line rectLine4 = Line(rectPoints[2].Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), rectPoints[3].Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                Point2D collisionPoint = Point2D();
                                float dist = INFINITY;
                                for (std::size_t w = 0; w < rectLines.size(); ++w) {
                                    Point2D crossPoint = Point2D();
                                    if (line1.Crosses(rectLines[w], crossPoint)) {
                                        if (line1.p2.Distance(crossPoint) < dist) {
                                            collisionPoint = crossPoint;
                                            dist = line1.p2.Distance(crossPoint);
                                        }
                                    }
                                }
                                if (std::isfinite(dist)) {
                                    Point2D cPointI = collisionPoint - objI->GetLocation();
                                    Point2D cPointIJ = collisionPoint - objIJ->GetLocation();
                                    objI->CollisionEnter(objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                    objIJ->CollisionEnter(objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                }

                            // Rect-Line Collision
                            } else if (shapesI[u].t == ShapeType::kRect && shapesIJ[v].t == ShapeType::kLine) {
                                Line line1 = Line(shapesIJ[v].shape.line.p1.Rotate(objIJ->GetRotation()) + objIJ->GetLocation(), 
                                                  shapesIJ[v].shape.line.p2.Rotate(objIJ->GetRotation()) + objIJ->GetLocation());
                                std::vector<Point2D> rectPoints = shapesI[u].shape.rect.Points();
                                Line rectLine1 = Line(rectPoints[0].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[1].Rotate(objI->GetRotation()) + objI->GetLocation());
                                Line rectLine2 = Line(rectPoints[0].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[2].Rotate(objI->GetRotation()) + objI->GetLocation()); 
                                Line rectLine3 = Line(rectPoints[1].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[3].Rotate(objI->GetRotation()) + objI->GetLocation()); 
                                Line rectLine4 = Line(rectPoints[2].Rotate(objI->GetRotation()) + objI->GetLocation(), rectPoints[3].Rotate(objI->GetRotation()) + objI->GetLocation());
                                std::vector<Line> rectLines = {rectLine1, rectLine2, rectLine3, rectLine4};
                                Point2D collisionPoint = Point2D();
                                float dist = INFINITY;
                                for (std::size_t w = 0; w < rectLines.size(); ++w) {
                                    Point2D crossPoint = Point2D();
                                    if (line1.Crosses(rectLines[w], crossPoint)) {
                                        if (line1.p2.Distance(crossPoint) < dist) {
                                            collisionPoint = crossPoint;
                                            dist = line1.p2.Distance(crossPoint);
                                        }
                                    }
                                }
                                if (std::isfinite(dist)) {
                                    Point2D cPointI = collisionPoint - objI->GetLocation();
                                    Point2D cPointIJ = collisionPoint - objIJ->GetLocation();
                                    objI->CollisionEnter(objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                    objIJ->CollisionEnter(objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                }
                            }
                        }
                    }                    
                }
            }
        }

        // 5. Post-collision updates.
        for (auto& object : mGameObjects)
        {
            mGameContext.CurrObject = object;
            object->LateUpdate(&mGameContext);
            mGameContext.CurrObject.reset();
        }

        // 6. Draw the scene, background first. Two separate passes keep the
        //    backgrounds (stars) underneath the foreground no matter what
        //    order the objects happen to sit in.
        mWindow->clear(sf::Color::Black);
        for (auto& object : mGameObjects)
        {
            // raw pointer here: the shared_ptr must not gain an owner.
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get()))
            {
                // The object draws in its own local space, so point the
                // DrawContext at its world position and rotation first.
                mDrawContext->SetContextOffset(graphics->GetLocation());
                mDrawContext->SetContextRotation(graphics->GetRotation());

                graphics->RenderBackground(&mGameContext);
            }
        }

        // 7. Foreground on top, each with its own transform.
        for (auto& object : mGameObjects)
        {
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get()))
            {
                mDrawContext->SetContextOffset(graphics->GetLocation());
                mDrawContext->SetContextRotation(graphics->GetRotation());

                graphics->RenderForeground(&mGameContext);
            }
        }

        // 8. UI pass: HUD content that ignores object transforms. Drawn last
        //    so it sits on top; the GUI context is never given an offset.
        for (auto& object : mGameObjects)
        {
            mGameContext.CurrObject = object;
            object->RenderUI(&mGameContext);
            mGameContext.CurrObject.reset();
        }

        mWindow->display();
        ++mGameContext.currentFrame;
    }
}

}  // namespace CMPUT350