
#include "NotificationManager.h"

#include <algorithm>

#include "GameObject.h"

namespace CMPUT350 {

//-----------------------------------------------------------------------
/**
 * @brief Subscribes an object to a notification name.
 *
 * Expired objects and empty keys are rejected, and the same object is never
 * added twice for the same key.
 * @param object Weak reference to the interested object.
 * @param key Notification name to listen for.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::Register(std::weak_ptr<GameObject> object, const std::string& key) {
    // Do not register an invalid object or an empty notification key.
    if (object.expired() || key.empty()) {
        return;
    }

    auto& listeners = mListeners[key];

    // Avoid registering the same object for the same key twice.
    for (const auto& listener : listeners) {
        if (!listener.owner_before(object) && !object.owner_before(listener)) {
            return;
        }
    }

    listeners.push_back(object);
}

//-----------------------------------------------------------------------
/**
 * @brief Removes an object's subscription for one notification.
 *
 * Expired listeners sharing the key are cleaned up at the same time, and the
 * key's entry is dropped once it has no listeners left.
 * @param object Weak reference to the object to remove.
 * @param key Notification name to stop listening for.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::Unregister(std::weak_ptr<GameObject> object, const std::string& key) {
    auto found = mListeners.find(key);

    if (found == mListeners.end()) {
        return;
    }

    auto& listeners = found->second;

    listeners.erase(std::remove_if(listeners.begin(), listeners.end(),
                                   [&](const std::weak_ptr<GameObject>& listener) {
                                       // Remove expired listeners as well as the requested object.
                                       const bool sameObject = !listener.owner_before(object) &&
                                                               !object.owner_before(listener);

                                       return listener.expired() || sameObject;
                                   }),
                    listeners.end());

    if (listeners.empty()) {
        mListeners.erase(found);
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Records and immediately delivers a notification.
 * @param message Notification name to send.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::Notify(const std::string& message) {
    if (mIsRecording && !mIsReplaying) {
        mRecordedEvents.push_back({mCurrentFrame - mRecordingStartFrame, message});
    }

    Deliver(message);
}

//-----------------------------------------------------------------------
/**
 * @brief Starts a fresh recording from the current frame.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::StartRecording() {
    mRecordedEvents.clear();
    mRecordingStartFrame = mCurrentFrame;
    mIsRecording = true;
    StopReplay();
}

//-----------------------------------------------------------------------
/**
 * @brief Stops recording; the captured events remain available.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::StopRecording() { mIsRecording = false; }

//-----------------------------------------------------------------------
/**
 * @brief Returns the events captured so far.
 * @return A reference to the recording buffer.
 */
//-----------------------------------------------------------------------
const std::vector<NotificationManager::RecordedEvent>& NotificationManager::GetRecordedEvents()
    const {
    return mRecordedEvents;
}

//-----------------------------------------------------------------------
/**
 * @brief Begins replaying the captured events from the current frame.
 * @return true if there was something to replay, false otherwise.
 */
//-----------------------------------------------------------------------
bool NotificationManager::StartReplay() {
    return StartReplay([](const std::string&) { return true; });
}

//-----------------------------------------------------------------------
/**
 * @brief Begins replaying the recorded events that pass a filter.
 *
 * Only the events accepted by the filter are copied into the playback buffer,
 * so a caller can choose to replay a subset (for example just player input).
 * The playback clock starts at the current frame, exactly like the unfiltered
 * overload.
 * @param filter Predicate returning true for events to replay.
 * @return true if at least one event was selected, false otherwise.
 */
//-----------------------------------------------------------------------
bool NotificationManager::StartReplay(const std::function<bool(const std::string&)>& filter) {
    mPlaybackEvents.clear();

    for (const auto& event : mRecordedEvents) {
        if (filter(event.message)) {
            mPlaybackEvents.push_back(event);
        }
    }

    if (mPlaybackEvents.empty()) {
        mIsReplaying = false;
        return false;
    }

    mPlaybackIndex = 0;
    mPlaybackStartFrame = mCurrentFrame;
    mIsReplaying = true;
    return true;
}

//-----------------------------------------------------------------------
/**
 * @brief Ends a replay and clears its playback buffer.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::StopReplay() {
    mIsReplaying = false;
    mPlaybackEvents.clear();
    mPlaybackIndex = 0;
}

//-----------------------------------------------------------------------
/**
 * @brief Reports whether a replay is running.
 * @return true while replaying.
 */
//-----------------------------------------------------------------------
bool NotificationManager::IsReplaying() const { return mIsReplaying; }

//-----------------------------------------------------------------------
/**
 * @brief Advances the internal clock and replays due notifications.
 *
 * Called once per frame by the engine. When a replay is active, every recorded
 * event whose offset is now due is delivered; the replay ends automatically
 * once the last event has been sent.
 * @param frame The engine's current frame number.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::AdvanceFrame(int frame) {
    mCurrentFrame = frame;
    if (!mIsReplaying) {
        return;
    }

    while (mPlaybackIndex < mPlaybackEvents.size() &&
           mPlaybackEvents[mPlaybackIndex].frame <= frame - mPlaybackStartFrame) {
        Deliver(mPlaybackEvents[mPlaybackIndex].message);
        ++mPlaybackIndex;
    }

    if (mPlaybackIndex == mPlaybackEvents.size()) {
        mIsReplaying = false;
        mPlaybackEvents.clear();
        mPlaybackIndex = 0;
    }
}

//-----------------------------------------------------------------------
/**
 * @brief Sends a message to every live listener registered for it.
 *
 * Expired and dead listeners are pruned first, then a snapshot of the list is
 * used for delivery so that a listener reacting by registering or unregistering
 * cannot invalidate the iteration.
 * @param message Notification name to deliver.
 * @return None.
 */
//-----------------------------------------------------------------------
void NotificationManager::Deliver(const std::string& message) {
    auto found = mListeners.find(message);

    if (found == mListeners.end()) {
        return;
    }

    auto& listeners = found->second;

    // Remove expired and dead listeners before delivering notifications.
    listeners.erase(std::remove_if(listeners.begin(), listeners.end(),
                                   [](const std::weak_ptr<GameObject>& listener) {
                                       const auto object = listener.lock();
                                       return !object || !object->IsAlive();
                                   }),
                    listeners.end());

    const auto listenersSnapshot = listeners;
    for (const auto& listener : listenersSnapshot) {
        if (auto object = listener.lock()) {
            if (object->IsAlive()) {
                object->ReceiveNotification(message);
            }
        }
    }
}

}  // namespace CMPUT350
