
#include "NotificationManager.h"

#include <algorithm>

#include "GameObject.h"

namespace CMPUT350 {

void NotificationManager::Register(
    std::weak_ptr<GameObject> object,
    const std::string& key)
{
    // Do not register an invalid object or an empty notification key.
    if (object.expired() || key.empty()) {
        return;
    }

    auto& listeners = mListeners[key];

    // Avoid registering the same object for the same key twice.
    for (const auto& listener : listeners) {
        if (!listener.owner_before(object) &&
            !object.owner_before(listener)) {
            return;
        }
    }

    listeners.push_back(object);
}

void NotificationManager::Unregister(
    std::weak_ptr<GameObject> object,
    const std::string& key)
{
    auto found = mListeners.find(key);

    if (found == mListeners.end()) {
        return;
    }

    auto& listeners = found->second;

    listeners.erase(
        std::remove_if(
            listeners.begin(),
            listeners.end(),
            [&](const std::weak_ptr<GameObject>& listener) {
                // Remove expired listeners as well as the requested object.
                const bool sameObject =
                    !listener.owner_before(object) &&
                    !object.owner_before(listener);

                return listener.expired() || sameObject;
            }),
        listeners.end()
    );

    if (listeners.empty()) {
        mListeners.erase(found);
    }
}

void NotificationManager::Notify(const std::string& message)
{
    if (mIsRecording && !mIsReplaying) {
        mRecordedEvents.push_back({mCurrentFrame - mRecordingStartFrame, message});
    }

    Deliver(message);
}

void NotificationManager::StartRecording()
{
    mRecordedEvents.clear();
    mRecordingStartFrame = mCurrentFrame;
    mIsRecording = true;
    StopReplay();
}

void NotificationManager::StopRecording()
{
    mIsRecording = false;
}

const std::vector<NotificationManager::RecordedEvent>&
NotificationManager::GetRecordedEvents() const
{
    return mRecordedEvents;
}

bool NotificationManager::StartReplay()
{
    if (mRecordedEvents.empty()) {
        return false;
    }

    mPlaybackEvents = mRecordedEvents;
    mPlaybackIndex = 0;
    mPlaybackStartFrame = mCurrentFrame;
    mIsReplaying = true;
    return true;
}

void NotificationManager::StopReplay()
{
    mIsReplaying = false;
    mPlaybackEvents.clear();
    mPlaybackIndex = 0;
}

bool NotificationManager::IsReplaying() const
{
    return mIsReplaying;
}

void NotificationManager::AdvanceFrame(int frame)
{
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

void NotificationManager::Deliver(const std::string& message)
{
    auto found = mListeners.find(message);

    if (found == mListeners.end()) {
        return;
    }

    auto& listeners = found->second;

    // Remove expired and dead listeners before delivering notifications.
    listeners.erase(
        std::remove_if(
            listeners.begin(),
            listeners.end(),
            [](const std::weak_ptr<GameObject>& listener) {
                const auto object = listener.lock();
                return !object || !object->IsAlive();
            }),
        listeners.end()
    );

    const auto listenersSnapshot = listeners;
    for (const auto& listener : listenersSnapshot) {
        if (auto object = listener.lock()) {
            if (object->IsAlive()) {
                object->ReceiveNotification(message);
            }
        }
    }
}

} // namespace CMPUT350
