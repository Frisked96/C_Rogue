#pragma once
#include "anatomy_components.hpp"
#include "component.hpp"
#include "components.hpp"
#include "inventory_component.hpp"
#include <bitset>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

constexpr size_t MAX_COMPONENTS = 32;
using Signature = std::bitset<MAX_COMPONENTS>;

class Entity;

class IEntityListener {
public:
  virtual void onEntitySignatureChanged(Entity *entity,
                                        Signature newSignature) = 0;
  virtual void onEntityMoved(Entity *entity, int oldX, int oldY, int newX,
                             int newY) = 0;
  virtual void onEntityDestroyed(Entity *entity) = 0;
  virtual ~IEntityListener() = default;
};

class Entity {
private:
  int id;
  std::unordered_map<Component::TypeId, std::unique_ptr<Component>> components;
  Signature signature;
  IEntityListener *listener = nullptr;

  static int next_id;

public:
  Entity();
  virtual ~Entity() = default;

  // getters
  int getId() const { return id; }
  Signature getSignature() const { return signature; }
  void setListener(IEntityListener *l) { listener = l; }

  void reset() {
    components.clear();
    signature.reset();
    id = next_id++;
    if (listener)
      listener->onEntitySignatureChanged(this, signature);
  }

  // component management
  template <typename T, typename... Args> T &addComponent(Args &&...args) {
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    auto &ref = *component;
    components[T::getStaticTypeId()] = std::move(component);

    size_t bit = T::getComponentTypeId();
    if (bit < MAX_COMPONENTS) {
      signature.set(bit);
      if (listener)
        listener->onEntitySignatureChanged(this, signature);
    }

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

    size_t bit = T::getComponentTypeId();
    if (bit < MAX_COMPONENTS) {
      signature.reset(bit);
      if (listener)
        listener->onEntitySignatureChanged(this, signature);
    }
  }

  // helper methods
  void setPosition(int x, int y);
  void move(int dx, int dy);
  char getGlyph() const;
  std::string getName() const;
  bool blocksMovement() const;
  bool hasHealth() const;
  HealthComponent *getHealth();
  bool hasAnatomy() const;
  AnatomyComponent *getAnatomy();
  bool hasCombat() const;
  CombatComponent *getCombat();
  bool hasInventory() const;
  InventoryComponent *getInventory();
  bool hasEnvironment() const;
  EnvironmentComponent *getEnvironment();
  bool hasSpatialProfile() const;
  SpatialProfileComponent *getSpatialProfile();
};