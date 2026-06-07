#include "entity_manager.hpp"
#include <algorithm>

EntityManager::EntityManager() {
    slots.reserve(1024);
}

EntityID EntityManager::spawn(EntityType type, int x, int y, int z) {
    uint32_t index;
    uint32_t gen;
    
    if (free_slots.empty()) {
        index = slots.size();
        gen = 1;
        slots.push_back({{}, gen, true});
    } else {
        index = free_slots.front();
        free_slots.pop_front();
        gen = ++slots[index].generation;
        slots[index].active = true;
    }

    Entity& e = slots[index].entity;
    e.id = make_id(index, gen);
    e.type = type;
    e.active = true;

    // Initialize state from properties
    const auto& p = e.props();
    e.state.x = x;
    e.state.y = y;
    e.state.z = z;
    e.state.height = p.base_height;
    e.state.weight = p.base_weight;
    
    e.state.hunger = 0.0f;
    e.state.thirst = 0.0f;
    e.state.energy_stored = p.max_calories_stored * 0.8f; // Start mostly full
    e.state.stomach_contents = 0.0f;
    e.state.excretion_buffer = 0.0f;
    
    // Convert daily needs to per-second rates
    e.state.hunger_rate = p.daily_calorie_need / 86400.0f;
    e.state.thirst_rate = 1.0f / (p.daily_water_need > 0 ? (86400.0f / p.daily_water_need) : 86400.0f);
    e.state.digestion_rate = p.digestion_rate;
    e.state.excretion_making_rate = p.excretion_making_rate;

    if (type == EntityType::PLAYER) {
        e.state.camera.active = true;
    }

    spatial_grid.add(e.id, x, y, z);
    return e.id;
}

void EntityManager::kill(EntityID id) {
    uint32_t index = get_index(id);
    if (index < slots.size() && slots[index].generation == get_generation(id)) {
        if (slots[index].active) {
            spatial_grid.remove(id, slots[index].entity.state.x, slots[index].entity.state.y, slots[index].entity.state.z);
            slots[index].active = false;
            free_slots.push_back(index);
        }
    }
}

Entity* EntityManager::get(EntityID id) {
    uint32_t index = get_index(id);
    if (index < slots.size() && slots[index].generation == get_generation(id) && slots[index].active) {
        return &slots[index].entity;
    }
    return nullptr;
}

const Entity* EntityManager::get(EntityID id) const {
    uint32_t index = get_index(id);
    if (index < slots.size() && slots[index].generation == get_generation(id) && slots[index].active) {
        return &slots[index].entity;
    }
    return nullptr;
}

void EntityManager::update(float dt) {
    for (auto& slot : slots) {
        if (!slot.active) continue;
        Entity& e = slot.entity;
        const auto& p = e.props();

        // 1. Metabolism
        float metabolic_drain = e.state.hunger_rate * dt;
        e.state.energy_stored -= metabolic_drain;

        // 2. Digestion
        if (e.state.stomach_contents > 0) {
            float digested = std::min(e.state.stomach_contents, e.state.digestion_rate * dt);
            e.state.stomach_contents -= digested;
            e.state.energy_stored += digested * 500.0f; // 1 unit food = 500 cal
            e.state.excretion_buffer += digested * e.state.excretion_making_rate;
        }

        // 3. Thirst
        e.state.thirst += e.state.thirst_rate * dt;

        // 4. Update Status Levels
        e.state.hunger = 1.0f - (e.state.energy_stored / p.max_calories_stored);
        
        // Clamping
        e.state.energy_stored = std::clamp(e.state.energy_stored, 0.0f, p.max_calories_stored);
        e.state.thirst = std::clamp(e.state.thirst, 0.0f, 1.0f);
        e.state.hunger = std::clamp(e.state.hunger, 0.0f, 1.0f);
        e.state.excretion_buffer = std::clamp(e.state.excretion_buffer, 0.0f, p.max_excretion_buffer);
        e.state.stomach_contents = std::clamp(e.state.stomach_contents, 0.0f, p.max_stomach);
    }
}

std::vector<Entity*> EntityManager::get_all_active() {
    std::vector<Entity*> active;
    for (auto& slot : slots) {
        if (slot.active) active.push_back(&slot.entity);
    }
    return active;
}
