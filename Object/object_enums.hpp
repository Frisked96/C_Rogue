#pragma once
#include <cstdint>

// Profession types for NPCs
enum class ProfessionType : uint8_t {
    NONE,
    FARMER,
    MERCHANT,
    GUARD,
    BLACKSMITH,
    HUNTER,
    WILD,       // For animals
};

// Simple AI goal state machine
enum class GoalType : uint8_t {
    IDLE,
    WANDER,
    SEEK_FOOD,
    SEEK_WATER,
    GO_TO_WORK,
    FLEE,
    ATTACK,
    TRADE,
    SLEEP,
};

// Event types for the subscribe/announce event bus
enum class EventType : uint8_t {
    OBJECT_SPAWNED,
    OBJECT_KILLED,
    OBJECT_MOVED,
    LIFE_STATE_CHANGED,
    HEALTH_CHANGED,
    TRADE_OCCURRED,
    INVENTORY_CHANGED,
    EVENT_TYPE_COUNT,  // Must be last - used for array sizing
};

// Tick frequency tiers
enum class TickFrequency : uint8_t {
    HIGH,       // Every turn
    MEDIUM,     // Every ~10 turns
    LOW,        // Every ~100 turns
};

// Object behavior type — drives which tick logic applies.
// Each behavior allocates its own sparse state on the instance.
enum class ObjectBehavior : uint8_t {
    INERT,       // No special tick (rocks, crates, items, corpses)
    VEGETATION,  // Absorbs water from soil, grows, can be harvested (trees, bushes)
    // Future: FUNGAL, AQUATIC, MINERAL_DEPOSIT, etc.
};
