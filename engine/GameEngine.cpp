#include "GameEngine.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "CollisionObject.h"
#include "DrawContext.h"
#include "FontData.h"
#include "GraphicsObject.h"

namespace CMPUT350 {

namespace {

//-----------------------------------------------------------------------
/**
 * @brief Finds where a directed line segment first enters a circle.
 *
 * The segment travels from p1 to p2, and a start point already inside the
 * circle is reported as the first contact.
 * @param line Segment to test, directed from p1 to p2.
 * @param circle Circle to test against.
 * @param entry Receives the first contact point in world coordinates.
 * @return true if the segment intersects the circle; otherwise false.
 */
//-----------------------------------------------------------------------
bool SegmentCircleEntry(const Line& line, const Circle& circle, Point2D& entry) {
    const Point2D direction = line.p2 - line.p1;
    const float lengthSquared = direction.Dot(direction);

    // A zero-length segment degenerates to a point test.
    if (lengthSquared <= 0.000001f) {
        if (line.p1.Distance(circle.center) <= circle.radius) {
            entry = line.p1;
            return true;
        }
        return false;
    }

    // Quadratic form |p1 + t*d - center|^2 = r^2 over the infinite line.
    const Point2D toStart = line.p1 - circle.center;
    const float c = toStart.Dot(toStart) - circle.radius * circle.radius;

    // Starting inside the circle means the first contact is the start point.
    if (c <= 0.0f) {
        entry = line.p1;
        return true;
    }

    const float b = 2.0f * toStart.Dot(direction);
    const float discriminant = b * b - 4.0f * lengthSquared * c;

    if (discriminant < 0.0f) {
        return false;
    }

    const float root = std::sqrt(discriminant);
    const float tNear = (-b - root) / (2.0f * lengthSquared);
    const float tFar = (-b + root) / (2.0f * lengthSquared);

    // tNear is the entry if it lands on the segment; otherwise the segment
    // ends inside the circle and tFar is the contact.
    if (tNear >= 0.0f && tNear <= 1.0f) {
        entry = line.p1 + direction * tNear;
        return true;
    }
    if (tFar >= 0.0f && tFar <= 1.0f) {
        entry = line.p1 + direction * tFar;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------
/**
 * @brief Finds where a directed segment first enters a rotated rectangle.
 *
 * The segment is transformed into the rectangle's local frame, where the
 * earliest crossing along p1 to p2 is selected. A start point inside the
 * rectangle is reported as the first contact.
 * @param line Segment to test, directed from p1 to p2 in world coordinates.
 * @param rect Rectangle in the object's local coordinates.
 * @param rectRotation Rectangle rotation in radians.
 * @param rectLocation Rectangle origin in world coordinates.
 * @param entry Receives the first contact point in world coordinates.
 * @return true if the segment intersects the rectangle; otherwise false.
 */
//-----------------------------------------------------------------------
bool SegmentRectEntry(const Line& line, const Rect& rect, float rectRotation,
                      const Point2D& rectLocation, Point2D& entry) {
    const Point2D localStart = (line.p1 - rectLocation).RotateReverse(rectRotation);
    const Point2D localEnd = (line.p2 - rectLocation).RotateReverse(rectRotation);

    if (rect.IsInside(localStart)) {
        entry = line.p1;
        return true;
    }

    const Line localLine(localStart, localEnd);
    const std::vector<Point2D> corners = rect.Points();
    const Line edges[4] = {
        Line(corners[0], corners[1]),  // top
        Line(corners[0], corners[2]),  // left
        Line(corners[1], corners[3]),  // right
        Line(corners[2], corners[3])   // bottom
    };

    const Point2D direction = localEnd - localStart;
    const float lengthSquared = direction.Dot(direction);

    bool found = false;
    float bestT = 2.0f;
    Point2D bestPoint;

    for (int i = 0; i < 4; ++i) {
        Point2D crossing;
        if (localLine.Crosses(edges[i], crossing)) {
            const float t = lengthSquared > 0.000001f
                                ? (crossing - localStart).Dot(direction) / lengthSquared
                                : 0.0f;

            // Keep the crossing closest to the start of the directed segment.
            if (t >= 0.0f && t <= 1.0f && t < bestT) {
                bestT = t;
                bestPoint = crossing;
                found = true;
            }
        }
    }

    if (!found) {
        return false;
    }

    entry = bestPoint.Rotate(rectRotation) + rectLocation;
    return true;
}

//-----------------------------------------------------------------------
/**
 * @brief Tests overlap between a circle and a rotated rectangle.
 *
 * The circle centre is transformed into the rectangle's local frame so the
 * rectangle can be treated as axis aligned.
 * @param circle Circle in world coordinates.
 * @param rect Rectangle in its object's local coordinates.
 * @param rectRotation Rectangle rotation in radians.
 * @param rectLocation Rectangle origin in world coordinates.
 * @return true if the shapes overlap; otherwise false.
 */
//-----------------------------------------------------------------------
bool CircleRectOverlap(const Circle& circle, const Rect& rect, float rectRotation,
                       Point2D rectLocation) {
    const Point2D local = (circle.center - rectLocation).RotateReverse(rectRotation);

    const float right = rect.topLeft.x + rect.width;
    const float bottom = rect.topLeft.y + rect.height;

    const float closestX = std::max(rect.topLeft.x, std::min(local.x, right));
    const float closestY = std::max(rect.topLeft.y, std::min(local.y, bottom));

    return Point2D(closestX, closestY).Distance(local) <= circle.radius;
}

//-----------------------------------------------------------------------
/**
 * @brief Tests overlap between two rotated rectangles using separating axes.
 *
 * Testing axes from both rectangles also detects full containment.
 * Implemented with the help of AI.
 * @param first First rectangle in its object's local coordinates.
 * @param firstRotation First rectangle's rotation in radians.
 * @param firstLocation First rectangle's origin in world coordinates.
 * @param second Second rectangle in its object's local coordinates.
 * @param secondRotation Second rectangle's rotation in radians.
 * @param secondLocation Second rectangle's origin in world coordinates.
 * @return true if the rectangles overlap; otherwise false.
 */
//-----------------------------------------------------------------------
bool RectRectOverlap(const Rect& first, float firstRotation, const Point2D& firstLocation,
                     const Rect& second, float secondRotation, const Point2D& secondLocation) {
    std::vector<Point2D> firstCorners = first.Points();
    std::vector<Point2D> secondCorners = second.Points();
    for (Point2D& corner : firstCorners) {
        corner = corner.Rotate(firstRotation) + firstLocation;
    }
    for (Point2D& corner : secondCorners) {
        corner = corner.Rotate(secondRotation) + secondLocation;
    }

    const Point2D edges[] = {firstCorners[1] - firstCorners[0], firstCorners[2] - firstCorners[0],
                             secondCorners[1] - secondCorners[0],
                             secondCorners[2] - secondCorners[0]};

    for (const Point2D& edge : edges) {
        const Point2D axis(-edge.y, edge.x);
        float firstMin = firstCorners[0].Dot(axis);
        float firstMax = firstMin;
        float secondMin = secondCorners[0].Dot(axis);
        float secondMax = secondMin;

        for (std::size_t i = 1; i < firstCorners.size(); ++i) {
            const float projection = firstCorners[i].Dot(axis);
            firstMin = std::min(firstMin, projection);
            firstMax = std::max(firstMax, projection);
        }
        for (std::size_t i = 1; i < secondCorners.size(); ++i) {
            const float projection = secondCorners[i].Dot(axis);
            secondMin = std::min(secondMin, projection);
            secondMax = std::max(secondMax, projection);
        }

        if (firstMax < secondMin || secondMax < firstMin) {
            return false;
        }
    }

    return true;
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
GameEngine::GameEngine(unsigned int width, unsigned int height, const std::string& name) {
    mWindow = std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), name);

    // Key repeat would send repeated key-down notifications instead of one.
    mWindow->setKeyRepeatEnabled(false);

    // The assignment caps the engine at 30 fps.
    mWindow->setFramerateLimit(30);

    // Load the embedded font and give it to the drawing context.
    mFont = std::make_shared<sf::Font>();
    // Warn but continue: the window is still usable without the font.
    if (!mFont->openFromMemory(_font, _font_len)) std::cerr << "WARNING: Font did not load.\n";

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
GameEngine::~GameEngine() {
    if (mWindow != nullptr && mWindow->isOpen()) mWindow->close();
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
void GameEngine::AddGameObject(std::shared_ptr<GameObject> gameObject) {
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
void GameEngine::InstallKeyDownNotification(int key, const std::string& notification) {
    if (notification.empty()) {
        return;
    }

    auto& notifications = mKeyDownNotifications[key];

    // The same key/action pair should be stored only once.
    if (std::find(notifications.begin(), notifications.end(), notification) ==
        notifications.end()) {
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
void GameEngine::InstallKeyUpNotification(int key, const std::string& notification) {
    if (notification.empty()) {
        return;
    }

    auto& notifications = mKeyUpNotifications[key];

    if (std::find(notifications.begin(), notifications.end(), notification) ==
        notifications.end()) {
        notifications.push_back(notification);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Removes every key binding registered with the engine.
 *
 * The keyboard setup screen calls this before re-installing the bindings it
 * wants, so a re-assigned key takes effect cleanly instead of both the old and
 * new keys sending notifications. Listeners registered with the notification
 * manager are unaffected; only the key-to-notification map is cleared.
 * @return None.
 */
//-----------------------------------------------------------------------
void GameEngine::ClearKeyNotifications() {
    mKeyDownNotifications.clear();
    mKeyUpNotifications.clear();
}

//-----------------------------------------------------------------------
/**
 * @brief Const iterator to the first live game object.
 * @return begin() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::begin() const { return mGameObjects.begin(); }

//-----------------------------------------------------------------------
/**
 * @brief Const iterator one past the last live game object.
 * @return end() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::end() const { return mGameObjects.end(); }

//-----------------------------------------------------------------------
/**
 * @brief Const iterator to the first live game object.
 * @return cbegin() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::cbegin() const { return mGameObjects.cbegin(); }

//-----------------------------------------------------------------------
/**
 * @brief Const iterator one past the last live game object.
 * @return cend() of the internal object vector.
 */
//-----------------------------------------------------------------------
GameEngine::const_iterator GameEngine::cend() const { return mGameObjects.cend(); }
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
void GameEngine::Run() {
    while (mWindow->isOpen()) {
        // 0. Remove dead objects. Swap with the last one so nothing is
        //    shifted; the loop deliberately does not advance i after a swap,
        //    because the element moved into position i is unchecked.
        for (std::size_t i = 0; i < mGameObjects.size();) {
            if (!mGameObjects[i]->IsAlive()) {
                mGameObjects[i] = mGameObjects.back();
                mGameObjects.pop_back();
            } else {
                ++i;
            }
        }

        // 1. Activate everything queued during the last frame.  Indexed loop
        //    because an object may add more objects while it initializes.
        for (std::size_t i = 0; i < mObjectsToAdd.size(); ++i) {
            mGameObjects.push_back(mObjectsToAdd[i]);
            mGameContext.CurrObject = mObjectsToAdd[i];
            mObjectsToAdd[i]->Initialize(&mGameContext);
            mGameContext.CurrObject.reset();
        }
        // Cleared only after the loop: Initialize above may have queued more.
        mObjectsToAdd.clear();

        mNotificationManager.AdvanceFrame(mGameContext.currentFrame);

        // 2. Window and keyboard events.
        while (const auto event = mWindow->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                mWindow->close();
                break;
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                const auto found =
                    mKeyDownNotifications.find(static_cast<int>(keyPressed->scancode));
                if (found != mKeyDownNotifications.end()) {
                    // Copy first: a listener may re-register (or clear) the
                    // engine's bindings while handling the notification, which
                    // would otherwise invalidate this iterator mid-loop.
                    const std::vector<std::string> notifications = found->second;
                    for (const auto& notification : notifications) {
                        mNotificationManager.Notify(notification);
                    }
                }
            } else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                const auto found =
                    mKeyUpNotifications.find(static_cast<int>(keyReleased->scancode));
                if (found != mKeyUpNotifications.end()) {
                    const std::vector<std::string> notifications = found->second;
                    for (const auto& notification : notifications) {
                        mNotificationManager.Notify(notification);
                    }
                }
            }
        }
        if (!mWindow->isOpen()) break;

        // 3. Simulate the game.
        for (auto& object : mGameObjects) {
            mGameContext.CurrObject = object;
            object->Update(&mGameContext);
            mGameContext.CurrObject.reset();
        }

        // 4. Detect collisions. The outer bound stops one short of the end and
        //    the inner loop starts at i + 1, so each pair is visited exactly
        //    once and no object is compared against itself.
        for (std::size_t i = 0; i + 1 < mGameObjects.size(); ++i) {
            // Objects not deriving from CollisionObject cannot collide.
            std::shared_ptr<CollisionObject> objI =
                std::dynamic_pointer_cast<CollisionObject>(mGameObjects[i]);
            if (objI == nullptr) continue;

            for (std::size_t j = i + 1; j < mGameObjects.size(); ++j) {
                std::shared_ptr<CollisionObject> objIJ =
                    std::dynamic_pointer_cast<CollisionObject>(mGameObjects[j]);
                if (objIJ == nullptr) continue;

                // Copy the bounds: operator&= mutates in place and we must
                // not disturb the object's own stored rectangle.
                Rect boundsI = objI->GetBounds();
                Rect boundsIJ = objIJ->GetBounds();

                // Rect::operator&= leaves width as NaN when the boxes miss,
                // so a real number here means they overlap.
                boundsI &= boundsIJ;
                if (!std::isnan(boundsI.width)) {
                    std::vector<Shape> shapesI = objI->GetShapes();
                    std::vector<Shape> shapesIJ = objIJ->GetShapes();

                    for (std::size_t u = 0; u < shapesI.size(); ++u) {
                        for (std::size_t v = 0; v < shapesIJ.size(); ++v) {
                            // Rect-Rect Collision
                            if (shapesI[u].t == ShapeType::kRect &&
                                shapesIJ[v].t == ShapeType::kRect) {
                                if (RectRectOverlap(shapesI[u].shape.rect, objI->GetRotation(),
                                                    objI->GetLocation(), shapesIJ[v].shape.rect,
                                                    objIJ->GetRotation(), objIJ->GetLocation())) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }
                                // Circle-Circle Collision
                            } else if (shapesI[u].t == ShapeType::kCircle &&
                                       shapesIJ[v].t == ShapeType::kCircle) {
                                Point2D center1 =
                                    shapesI[u].shape.circle.center.Rotate(objI->GetRotation()) +
                                    objI->GetLocation();
                                Point2D center2 =
                                    shapesIJ[v].shape.circle.center.Rotate(objIJ->GetRotation()) +
                                    objIJ->GetLocation();
                                if (center1.Distance(center2) <=
                                    shapesI[u].shape.circle.radius +
                                        shapesIJ[v].shape.circle.radius) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }

                                // Line-Line Collision
                            } else if (shapesI[u].t == ShapeType::kLine &&
                                       shapesIJ[v].t == ShapeType::kLine) {
                                Point2D crossPoint = Point2D();
                                Line line1 =
                                    Line(shapesI[u].shape.line.p1.Rotate(objI->GetRotation()) +
                                             objI->GetLocation(),
                                         shapesI[u].shape.line.p2.Rotate(objI->GetRotation()) +
                                             objI->GetLocation());
                                Line line2 =
                                    Line(shapesIJ[v].shape.line.p1.Rotate(objIJ->GetRotation()) +
                                             objIJ->GetLocation(),
                                         shapesIJ[v].shape.line.p2.Rotate(objIJ->GetRotation()) +
                                             objIJ->GetLocation());
                                if (line1.Crosses(line2, crossPoint)) {
                                    Point2D cPointI = crossPoint - objI->GetLocation();
                                    Point2D cPointIJ = crossPoint - objIJ->GetLocation();
                                    objI->CollisionEnter(
                                        objIJ, cPointI.RotateReverse(objI->GetRotation()));
                                    objIJ->CollisionEnter(
                                        objI, cPointIJ.RotateReverse(objIJ->GetRotation()));
                                }

                                // Circle-Line Collision
                            } else if (shapesI[u].t == ShapeType::kCircle &&
                                       shapesIJ[v].t == ShapeType::kLine) {
                                Circle circ1 = Circle(
                                    shapesI[u].shape.circle.center.Rotate(objI->GetRotation()) +
                                        objI->GetLocation(),
                                    shapesI[u].shape.circle.radius);
                                Line line1 =
                                    Line(shapesIJ[v].shape.line.p1.Rotate(objIJ->GetRotation()) +
                                             objIJ->GetLocation(),
                                         shapesIJ[v].shape.line.p2.Rotate(objIJ->GetRotation()) +
                                             objIJ->GetLocation());
                                Point2D collisionPoint;
                                if (SegmentCircleEntry(line1, circ1, collisionPoint)) {
                                    Point2D cPointI = (collisionPoint - objI->GetLocation())
                                                          .RotateReverse(objI->GetRotation());
                                    Point2D cPointIJ = (collisionPoint - objIJ->GetLocation())
                                                           .RotateReverse(objIJ->GetRotation());
                                    objI->CollisionEnter(objIJ, cPointI);
                                    objIJ->CollisionEnter(objI, cPointIJ);
                                }

                                // Line-Circle Collision
                            } else if (shapesI[u].t == ShapeType::kLine &&
                                       shapesIJ[v].t == ShapeType::kCircle) {
                                Circle circ1 = Circle(
                                    shapesIJ[v].shape.circle.center.Rotate(objIJ->GetRotation()) +
                                        objIJ->GetLocation(),
                                    shapesIJ[v].shape.circle.radius);
                                Line line1 =
                                    Line(shapesI[u].shape.line.p1.Rotate(objI->GetRotation()) +
                                             objI->GetLocation(),
                                         shapesI[u].shape.line.p2.Rotate(objI->GetRotation()) +
                                             objI->GetLocation());
                                Point2D collisionPoint;
                                if (SegmentCircleEntry(line1, circ1, collisionPoint)) {
                                    Point2D cPointI = (collisionPoint - objI->GetLocation())
                                                          .RotateReverse(objI->GetRotation());
                                    Point2D cPointIJ = (collisionPoint - objIJ->GetLocation())
                                                           .RotateReverse(objIJ->GetRotation());
                                    objI->CollisionEnter(objIJ, cPointI);
                                    objIJ->CollisionEnter(objI, cPointIJ);
                                }

                                // Circle-Rect Collision
                            } else if (shapesI[u].t == ShapeType::kCircle &&
                                       shapesIJ[v].t == ShapeType::kRect) {
                                Circle circ1 = Circle(
                                    shapesI[u].shape.circle.center.Rotate(objI->GetRotation()) +
                                        objI->GetLocation(),
                                    shapesI[u].shape.circle.radius);
                                if (CircleRectOverlap(circ1, shapesIJ[v].shape.rect,
                                                      objIJ->GetRotation(), objIJ->GetLocation())) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }

                                // Rect-Circle Collision
                            } else if (shapesI[u].t == ShapeType::kRect &&
                                       shapesIJ[v].t == ShapeType::kCircle) {
                                Circle circ1 = Circle(
                                    shapesIJ[v].shape.circle.center.Rotate(objIJ->GetRotation()) +
                                        objIJ->GetLocation(),
                                    shapesIJ[v].shape.circle.radius);
                                if (CircleRectOverlap(circ1, shapesI[u].shape.rect,
                                                      objI->GetRotation(), objI->GetLocation())) {
                                    objI->CollisionEnter(objIJ);
                                    objIJ->CollisionEnter(objI);
                                }

                                // Line-Rect Collision
                            } else if (shapesI[u].t == ShapeType::kLine &&
                                       shapesIJ[v].t == ShapeType::kRect) {
                                Line line1 =
                                    Line(shapesI[u].shape.line.p1.Rotate(objI->GetRotation()) +
                                             objI->GetLocation(),
                                         shapesI[u].shape.line.p2.Rotate(objI->GetRotation()) +
                                             objI->GetLocation());
                                Point2D collisionPoint;
                                if (SegmentRectEntry(line1, shapesIJ[v].shape.rect,
                                                     objIJ->GetRotation(), objIJ->GetLocation(),
                                                     collisionPoint)) {
                                    Point2D cPointI = (collisionPoint - objI->GetLocation())
                                                          .RotateReverse(objI->GetRotation());
                                    Point2D cPointIJ = (collisionPoint - objIJ->GetLocation())
                                                           .RotateReverse(objIJ->GetRotation());
                                    objI->CollisionEnter(objIJ, cPointI);
                                    objIJ->CollisionEnter(objI, cPointIJ);
                                }

                                // Rect-Line Collision
                            } else if (shapesI[u].t == ShapeType::kRect &&
                                       shapesIJ[v].t == ShapeType::kLine) {
                                Line line1 =
                                    Line(shapesIJ[v].shape.line.p1.Rotate(objIJ->GetRotation()) +
                                             objIJ->GetLocation(),
                                         shapesIJ[v].shape.line.p2.Rotate(objIJ->GetRotation()) +
                                             objIJ->GetLocation());
                                Point2D collisionPoint;
                                if (SegmentRectEntry(line1, shapesI[u].shape.rect,
                                                     objI->GetRotation(), objI->GetLocation(),
                                                     collisionPoint)) {
                                    Point2D cPointI = (collisionPoint - objI->GetLocation())
                                                          .RotateReverse(objI->GetRotation());
                                    Point2D cPointIJ = (collisionPoint - objIJ->GetLocation())
                                                           .RotateReverse(objIJ->GetRotation());
                                    objI->CollisionEnter(objIJ, cPointI);
                                    objIJ->CollisionEnter(objI, cPointIJ);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 5. Post-collision updates.
        for (auto& object : mGameObjects) {
            mGameContext.CurrObject = object;
            object->LateUpdate(&mGameContext);
            mGameContext.CurrObject.reset();
        }

        // 6. Draw the scene, background first. Two separate passes keep the
        //    backgrounds (stars) underneath the foreground no matter what
        //    order the objects happen to sit in.
        mWindow->clear(sf::Color::Black);
        for (auto& object : mGameObjects) {
            // raw pointer here: the shared_ptr must not gain an owner.
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get())) {
                // The object draws in its own local space, so point the
                // DrawContext at its world position and rotation first.
                mDrawContext->SetContextOffset(graphics->GetLocation());
                mDrawContext->SetContextRotation(graphics->GetRotation());

                graphics->RenderBackground(&mGameContext);
            }
        }

        // 7. Foreground on top, each with its own transform.
        for (auto& object : mGameObjects) {
            if (auto* graphics = dynamic_cast<GraphicsObject*>(object.get())) {
                mDrawContext->SetContextOffset(graphics->GetLocation());
                mDrawContext->SetContextRotation(graphics->GetRotation());

                graphics->RenderForeground(&mGameContext);
            }
        }

        // 8. UI pass: HUD content that ignores object transforms. Drawn last
        //    so it sits on top; the GUI context is never given an offset.
        for (auto& object : mGameObjects) {
            mGameContext.CurrObject = object;
            object->RenderUI(&mGameContext);
            mGameContext.CurrObject.reset();
        }

        mWindow->display();
        ++mGameContext.currentFrame;
    }
}

}  // namespace CMPUT350