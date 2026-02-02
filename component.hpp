#pragma once
#include <memory>
#include <typeindex>

// base component class- all components inherit from this
class Component {
public:
  virtual ~Component() = default;
  // unique id for each component
  using TypeId = std::type_index;
  virtual TypeId getTypeId() const = 0;

  // for cloning components
  virtual std::unique_ptr<Component> clone() const = 0;
};

// helper for registering component types
template <typename T> class BaseComponent : public Component {
public:
  static TypeId getStaticId() { return std::type_index(typeid(T)); }

  TypeId getTypeId() const override { return getStaticTypeId(); }

  // default clone imp
  std::unique_ptr<Component> clone() const override {
    return std::make_unique<T>(*static_cast<const T *>(this));
  }
};
