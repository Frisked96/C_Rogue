#include "event_bus.hpp"
#include <algorithm>

uint32_t EventBus::subscribe(EventType type, EventCallback callback) {
  uint32_t id = next_sub_id_++;
  subscribers_[static_cast<size_t>(type)].push_back({id, std::move(callback)});
  return id;
}

void EventBus::unsubscribe(EventType type, uint32_t sub_id) {
  auto &subs = subscribers_[static_cast<size_t>(type)];
  subs.erase(std::remove_if(
                 subs.begin(), subs.end(),
                 [sub_id](const Subscription &s) { return s.id == sub_id; }),
             subs.end());
}

void EventBus::announce(EventType type, const EventPayload &payload) {
  dirty_flags_[static_cast<size_t>(type)] = true;
  for (auto &sub : subscribers_[static_cast<size_t>(type)]) {
    sub.callback(payload);
  }
}

bool EventBus::is_dirty(EventType type) const {
  return dirty_flags_[static_cast<size_t>(type)];
}

void EventBus::clear_dirty() { dirty_flags_.fill(false); }

void EventBus::clear_dirty(EventType type) {
  dirty_flags_[static_cast<size_t>(type)] = false;
}
