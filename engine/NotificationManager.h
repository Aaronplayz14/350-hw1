#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CMPUT350 {

class GameObject;

class NotificationManager {
public:
    struct RecordedEvent {
        int frame;
        std::string message;
    };

    // Subscribe an object to a named notification.
    void Register(std::weak_ptr<GameObject> object, const std::string& key);

    // Remove a subscription.
    void Unregister(std::weak_ptr<GameObject> object, const std::string& key);

    // Send a notification to all currently subscribed objects.
    void Notify(const std::string& message);

    void StartRecording();
    void StopRecording();
    const std::vector<RecordedEvent>& GetRecordedEvents() const;

    bool StartReplay();
    void StopReplay();
    bool IsReplaying() const;
    void AdvanceFrame(int frame);

private:
    void Deliver(const std::string& message);

    std::unordered_map<std::string, std::vector<std::weak_ptr<GameObject>>> mListeners;
    std::vector<RecordedEvent> mRecordedEvents;
    std::vector<RecordedEvent> mPlaybackEvents;
    int mCurrentFrame = 0;
    int mRecordingStartFrame = 0;
    int mPlaybackStartFrame = 0;
    std::size_t mPlaybackIndex = 0;
    bool mIsRecording = true;
    bool mIsReplaying = false;
};

}  // namespace CMPUT350

#endif