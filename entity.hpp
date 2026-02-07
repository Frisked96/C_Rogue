#pragma once
#include "component.hpp"
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

class Entity {
private:
  int id;
  std::unordered_map<Component::TypeId, std::unique_ptr<Component>> components;

  static int next_id;

public:
  Entity();

  // getters
  int getId() const { return id; }

  // component management
  template <typename T, typename... Args> T &addComponent(Args &&...args) {
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    auto &ref = *component;
    components[T::getStaticTypeId()] = std::move(component);
    return ref;
  }

  template <typename T> bool hasComponent() const {
    return components.find(T::getStaticTypeId()) != components.end();
  }

  template <typename T> T *getComponent() {
    auto it = components.find(T::getStaticTypeId());
    if (it != components.end()) {
      return static_cast<T *>(it->second.get());
    }
    return nullptr;
  }

  template <typename T> const T *getComponent() const {
    auto it = components.find(T::getStaticTypeId());
    if (it != components.end()) {
      return static_cast<const T *>(it->second.get());
    }
    return nullptr;
  }

  template <typename T> void removeComponent() {
    components.erase(T::getStaticTypeId());
  }

  // helper methods
  void setPosition(int x, int y);
  void move(int dx, int dy);
  char getGlyph() const;
  std::string getName() const;
  bool blocksMovement() const;
};