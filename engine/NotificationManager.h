#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CMPUT350 {

class GameObject;

//-----------------------------------------------------------------------
/**
 * @brief Decouples game objects that need to talk to each other.
 *
 * Objects register an interest in a named message, and any code can then ask
 * the manager to send that message. The manager delivers it immediately to
 * every object still listening for that name. Listeners are stored as weak
 * pointers, so the manager never keeps a dead object alive and quietly drops
 * objects that have expired.
 *
 * The manager can also record every notification with the frame it happened on
 * and replay them later, which is the basis of the replay extra credit.
 */
//-----------------------------------------------------------------------
class NotificationManager {
public:
    /// @brief One recorded notification together with its frame offset.
    struct RecordedEvent {
        int frame;
        std::string message;
    };

    /// @brief Subscribe an object to a named notification.
    void Register(std::weak_ptr<GameObject> object, const std::string& key);

    /// @brief Remove a subscription.
    void Unregister(std::weak_ptr<GameObject> object, const std::string& key);

    /// @brief Send a notification to all currently subscribed objects.
    void Notify(const std::string& message);

    /// @brief Begin capturing notifications (clears any previous recording).
    void StartRecording();
    /// @brief Stop capturing notifications.
    void StopRecording();
    /// @brief Access the captured notifications.
    const std::vector<RecordedEvent>& GetRecordedEvents() const;

    /// @brief Begin replaying the recorded notifications; false if none exist.
    bool StartReplay();
    /// @brief Stop any active replay and discard its playback buffer.
    void StopReplay();
    /// @brief Whether a replay is currently in progress.
    bool IsReplaying() const;
    /// @brief Deliver any recorded notifications that are due at this frame.
    void AdvanceFrame(int frame);

private:
    /// @brief Deliver a message to every live listener registered for it.
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
