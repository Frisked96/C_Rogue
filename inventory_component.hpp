#pragma once
#include "component.hpp"
#include <vector>
#include <memory>
#include <string>

// Forward declaration
class Item;

class InventoryComponent : public BaseComponent<InventoryComponent> {
public:
    struct Slot {
        std::unique_ptr<Item> item;
        int quantity;
    };
    
    std::vector<Slot> items;
    int capacity;  // Max number of different items
    int weight_capacity;  // Max weight
    
    InventoryComponent(int cap = 20, int weight_cap = 100)
        : capacity(cap), weight_capacity(weight_cap) {}
    
    // Add an item to inventory
    bool addItem(std::unique_ptr<Item> item, int quantity = 1);
    
    // Remove an item
    bool removeItem(const std::string& item_name, int quantity = 1);
    
    // Check if inventory has item
    bool hasItem(const std::string& item_name) const;
    
    // Get total weight of inventory
    int getTotalWeight() const;
    
    // Check if there's space for new item
    bool hasSpace() const {
        return items.size() < capacity;
    }
};