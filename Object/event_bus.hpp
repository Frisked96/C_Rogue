#pragma once
#include "object_enums.hpp"
#include <cstdint>
#include <functional>
#include <vector>
#include <array>

// Payload for event callbacks
struct EventPayload {
    uint32_t source_uid = 0;   // UID of the object that caused the event
    uint32_t target_uid = 0;   // UID of the target object (if applicable)
    float value = 0.0f;        // Generic numeric data
};

using EventCallback = std::function<void(const EventPayload&)>;

// Lightweight subscribe/announce event bus with dirty flags.
// Systems subscribe to events they care about. When an event is announced,
// all subscribers are notified AND a dirty flag is set. Systems can check
// dirty flags to decide whether expensive updates are needed this turn.
class EventBus {
private:
    static constexpr size_t TYPE_COUNT = static_cast<size_t>(EventType::EVENT_TYPE_COUNT);

    struct Subscription {
        uint32_t id;
        EventCallback callback;
    };

    std::array<std::vector<Subscription>, TYPE_COUNT> subscribers_;
    std::array<bool, TYPE_COUNT> dirty_flags_{};
    uint32_t next_sub_id_ = 1;

public:
    EventBus() = default;

    // Subscribe to an event type. Returns a subscription ID for unsubscribing.
    uint32_t subscribe(EventType type, EventCallback callback);

    // Unsubscribe by ID.
    void unsubscribe(EventType type, uint32_t sub_id);

    // Fire an event: notifies all subscribers AND sets the dirty flag.
    void announce(EventType type, const EventPayload& payload = {});

    // Check if any event of this type was fired since last clear.
    bool is_dirty(EventType type) const;

    // Reset all dirty flags. Call at end of each turn.
    void clear_dirty();

    // Reset a single dirty flag.
    void clear_dirty(EventType type);
};
