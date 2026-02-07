#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "event.hpp"

/**
 * @brief Priority levels for events.
 * Higher priority events are processed first.
 */
enum class EventPriority {
  LOW = 0,
  NORMAL = 1,
  HIGH = 2,
  CRITICAL = 3
};

/**
 * @brief Internal interface for event handlers to allow polymorphic storage.
 */
class IEventHandler {
public:
  virtual ~IEventHandler() = default;
  virtual void exec(const Event &event) = 0;
};

/**
 * @brief Concrete event handler for a specific event type.
 */
template <typename T> class EventHandler : public IEventHandler {
public:
  using Callback = std::function<void(const T &)>;
  
  EventHandler(Callback cb) : callback(cb) {}
  
  void exec(const Event &event) override {
    callback(static_cast<const T &>(event));
  }

private:
  Callback callback;
};

/**
 * @brief Internal structure for events waiting in the queue.
 */
struct QueuedEvent {
  std::unique_ptr<Event> event;
  std::type_index type;
  EventPriority priority;
  size_t sequence;

  QueuedEvent(std::unique_ptr<Event> e, std::type_index t, EventPriority p,
              size_t s)
      : event(std::move(e)), type(t), priority(p), sequence(s) {}

  // Comparison for the priority queue (max-heap)
  bool operator<(const QueuedEvent &other) const {
    if (priority != other.priority) {
      return priority < other.priority;
    }
    // For same priority, use sequence to maintain FIFO (smaller sequence = earlier)
    // std::push_heap/pop_heap creates a max-heap, so we want the smallest sequence
    // to be "greater" in priority if we want it processed first.
    return sequence > other.sequence; 
  }
};

/**
 * @brief Central event bus for decoupled communication.
 */
class EventBus {
private:
  std::unordered_map<std::type_index,
                     std::vector<std::unique_ptr<IEventHandler>>>
      subscribers;
  std::vector<QueuedEvent> eventQueue;
  size_t nextSequence = 0;

public:
  /**
   * @brief Subscribes a callback function to an event type.
   * @tparam T The event type to subscribe to.
   * @param callback The function to call when the event occurs.
   */
  template <typename T> void subscribe(std::function<void(const T &)> callback) {
    subscribers[typeid(T)].push_back(std::make_unique<EventHandler<T>>(callback));
  }

  /**
   * @brief Subscribes a member function of an instance to an event type.
   * @tparam Instance The class type of the instance.
   * @tparam T The event type to subscribe to.
   * @param instance Pointer to the class instance.
   * @param memberFunction Pointer to the member function.
   */
  template <typename T, typename Instance>
  void subscribe(Instance *instance, void (Instance::*memberFunction)(const T &)) {
    subscribe<T>([instance, memberFunction](const T &event) {
      (instance->*memberFunction)(event);
    });
  }

  /**
   * @brief Dispatches an event immediately to all subscribers.
   * @tparam T The event type.
   * @param event The event data.
   */
  template <typename T> void dispatch(const T &event) {
    auto it = subscribers.find(typeid(T));
    if (it != subscribers.end()) {
      for (auto &handler : it->second) {
        handler->exec(event);
      }
    }
  }

  /**
   * @brief Queues an event for later processing.
   * @tparam T The event type.
   * @tparam Args Argument types for T's constructor.
   * @param priority The priority of the event.
   * @param args Arguments to construct the event.
   */
  template <typename T, typename... Args>
  void publish(EventPriority priority, Args &&...args) {
    eventQueue.emplace_back(std::make_unique<T>(std::forward<Args>(args)...),
                            typeid(T), priority, nextSequence++);
    std::push_heap(eventQueue.begin(), eventQueue.end());
  }

  /**
   * @brief Processes all queued events in order of priority and sequence.
   */
  void processQueue() {
    while (!eventQueue.empty()) {
      std::pop_heap(eventQueue.begin(), eventQueue.end());
      QueuedEvent queued = std::move(eventQueue.back());
      eventQueue.pop_back();

      auto it = subscribers.find(queued.type);
      if (it != subscribers.end()) {
        for (auto &handler : it->second) {
          handler->exec(*queued.event);
        }
      }
    }
  }

  /**
   * @brief Clears all subscribers and the event queue.
   */
  void clear() {
    subscribers.clear();
    eventQueue.clear();
    nextSequence = 0;
  }
};
