#pragma once

#include <string>
#include <memory>
#include <vector>
#include "combat_types.hpp"

enum class ItemType {
    WEAPON,
    ARMOR,
    CONSUMABLE,
    REAGENT,
    TOOL,
    MISC
};

enum class ItemRarity {
    COMMON,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY
};

class Item {
protected:
    std::string name;
    std::string description;
    ItemType type;
    ItemRarity rarity;
    int weight;
    int value;

public:
    Item(const std::string& name, ItemType type, int weight = 1, int value = 10)
        : name(name), type(type), weight(weight), value(value), 
          description(""), rarity(ItemRarity::COMMON) {}

    virtual ~Item() = default;

    // Getters
    std::string getName() const { return name; }
    std::string getDescription() const { return description; }
    ItemType getType() const { return type; }
    ItemRarity getRarity() const { return rarity; }
    int getWeight() const { return weight; }
    int getValue() const { return value; }

    // Setters
    void setDescription(const std::string& desc) { description = desc; }
    void setRarity(ItemRarity r) { rarity = r; }

    // Deep copy
    virtual std::unique_ptr<Item> clone() const {
        return std::make_unique<Item>(*this);
    }
};

class Weapon : public Item {
public:
    float damage;
    DamageType damage_type;
    float reach;           // Physical length (e.g., 1.0 for a longsword, 0.3 for a dagger)
    float leverage_bonus;  // Multiplier for accuracy or strength-based interactions
    float speed_penalty;   // Larger weapons might be slower to swing

    Weapon(const std::string& name, float dmg, DamageType type = DamageType::SLASHING, float rch = 1.0f, int weight = 5)
        : Item(name, ItemType::WEAPON, weight), damage(dmg), damage_type(type), 
          reach(rch), leverage_bonus(1.0f), speed_penalty(0.0f) {}

    std::unique_ptr<Item> clone() const override {
        return std::make_unique<Weapon>(*this);
    }
};

class Armor : public Item {
public:
    int defense_power;
    std::string slot; // "head", "torso", "legs", etc.

    Armor(const std::string& name, int def, const std::string& slot = "torso", int weight = 10)
        : Item(name, ItemType::ARMOR, weight), defense_power(def), slot(slot) {}

    std::unique_ptr<Item> clone() const override {
        return std::make_unique<Armor>(*this);
    }
};

class Consumable : public Item {
public:
    int potency;
    int charges;

    Consumable(const std::string& name, int potency, int charges = 1, int weight = 1)
        : Item(name, ItemType::CONSUMABLE, weight), potency(potency), charges(charges) {}

    std::unique_ptr<Item> clone() const override {
        return std::make_unique<Consumable>(*this);
    }

    virtual void onUse() {
        if (charges > 0) charges--;
    }
};
