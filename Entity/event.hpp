#pragma once
#include <typeindex>

/**
 * @brief Base class for all events in the system.
 */
struct Event {
  virtual ~Event() = default;
};

/**
 * @brief Helper template to create concrete event types.
 * 
 * Usage:
 * struct MyEvent : public BaseEvent<MyEvent> {
 *   int data;
 * };
 */
template <typename T> struct BaseEvent : public Event {
  static std::type_index getStaticType() { return std::type_index(typeid(T)); }
};
