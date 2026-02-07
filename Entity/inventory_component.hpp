#pragma once
#include "component.hpp"
#include <memory>
#include <string>
#include <vector>

// Define Item to allow deep copying
class Item {
public:
  virtual ~Item() = default;
  virtual std::unique_ptr<Item> clone() const {
    return std::make_unique<Item>(*this);
  }
  // Add other virtual methods if needed for the game
};

class InventoryComponent : public BaseComponent<InventoryComponent> {
public:
  struct Slot {
    std::unique_ptr<Item> item;
    int quantity;

    Slot() : quantity(0) {}
    Slot(std::unique_ptr<Item> i, int q) : item(std::move(i)), quantity(q) {}

    // Deep copy constructor
    Slot(const Slot &other) : quantity(other.quantity) {
      if (other.item) {
        item = other.item->clone();
      }
    }

    // Move constructor
    Slot(Slot &&other) noexcept = default;
    // Copy assignment
    Slot &operator=(const Slot &other) {
      if (this != &other) {
        quantity = other.quantity;
        if (other.item)
          item = other.item->clone();
        else
          item.reset();
      }
      return *this;
    }
    // Move assignment
    Slot &operator=(Slot &&other) noexcept = default;
  };

  std::vector<Slot> items;
  int capacity;        // Max number of different items
  int weight_capacity; // Max weight

  InventoryComponent(int cap = 20, int weight_cap = 100)
      : capacity(cap), weight_capacity(weight_cap) {}

  // Add an item to inventory
  bool addItem(std::unique_ptr<Item> item, int quantity = 1);

  // Remove an item
  bool removeItem(const std::string &item_name, int quantity = 1);

  // Check if inventory has item
  bool hasItem(const std::string &item_name) const;

  // Get total weight of inventory
  int getTotalWeight() const;

  // Check if there's space for new item
  bool hasSpace() const { return items.size() < capacity; }
};