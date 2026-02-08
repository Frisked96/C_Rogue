#pragma once
#include <memory>
#include <typeindex>
#include <vector>

// Forward declaration
using ComponentTypeID = std::size_t;

// base component class- all components inherit from this
class Component {
public:
  virtual ~Component() = default;
  // unique id for each component
  using TypeId = std::type_index;
  virtual TypeId getTypeId() const = 0;

  // for cloning components
  virtual std::unique_ptr<Component> clone() const = 0;

protected:
  static ComponentTypeID getNextTypeId() {
    static ComponentTypeID typeIdCounter = 0;
    return typeIdCounter++;
  }
};

// helper for registering component types
template <typename T> class BaseComponent : public Component {
public:
  static TypeId getStaticTypeId() { return std::type_index(typeid(T)); }

  // Integer ID for bitsets
  static ComponentTypeID getComponentTypeId() {
    static ComponentTypeID id = getNextTypeId();
    return id;
  }

  TypeId getTypeId() const override { return getStaticTypeId(); }

  // default clone implementation returns nullptr. Subclasses with data should
  // override if cloning is needed.
  std::unique_ptr<Component> clone() const override { return nullptr; }
};
