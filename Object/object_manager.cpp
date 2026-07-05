#include "object_manager.hpp"
#include <algorithm>
#include <cmath>
#include "../game_map.hpp"

ObjectManager::ObjectManager(ObjectPrototypeDB& db, EventBus& bus)
    : proto_db_(db), event_bus_(bus) {
    slots_.reserve(1024);
}

ObjectUID ObjectManager::spawn(uint16_t prototype_id, int x, int y, int z) {
    const auto& proto = proto_db_.get(prototype_id);

    uint32_t index;
    uint32_t gen;

    if (free_slots_.empty()) {
        index = static_cast<uint32_t>(slots_.size());
        gen = 1;
        slots_.push_back({});
    } else {
        index = free_slots_.front();
        free_slots_.pop_front();
        gen = ++slots_[index].generation;
    }

    Slot& slot = slots_[index];
    slot.occupied = true;
    slot.generation = gen;

    ObjectInstance& obj = slot.obj;
    obj.uid = make_uid(index, gen);
    obj.prototype_id = prototype_id;
    obj.x = x;
    obj.y = y;
    obj.z = z;
    obj.health = proto.max_health;
    obj.owner_uid = INVALID_OBJECT_UID;
    obj.active = true;
    obj.inventory.clear();

    // Allocate LifeState only for living objects
    if (proto.is_living) {
        obj.life = std::make_unique<LifeState>();
        obj.life->profession = proto.default_job;
    } else {
        obj.life.reset();
    }

    // Allocate VegetationState for vegetation-behavior objects
    if (proto.behavior == ObjectBehavior::VEGETATION) {
        obj.vegetation = std::make_unique<VegetationState>();
    } else {
        obj.vegetation.reset();
    }

    spatial_grid_.add(obj.uid, x, y, z);

    event_bus_.announce(EventType::OBJECT_SPAWNED,
                        {obj.uid, 0, static_cast<float>(prototype_id)});

    return obj.uid;
}

void ObjectManager::kill(ObjectUID uid) {
    uint32_t index = uid_index(uid);
    if (index >= slots_.size()) return;

    Slot& slot = slots_[index];
    if (!slot.occupied || slot.generation != uid_gen(uid)) return;

    spatial_grid_.remove(uid, slot.obj.x, slot.obj.y, slot.obj.z);

    event_bus_.announce(EventType::OBJECT_KILLED, {uid, 0, 0.0f});

    slot.obj.active = false;
    slot.obj.life.reset();
    slot.obj.vegetation.reset();
    slot.obj.inventory.clear();
    slot.occupied = false;
    free_slots_.push_back(index);
}

ObjectInstance* ObjectManager::get(ObjectUID uid) {
    uint32_t index = uid_index(uid);
    if (index >= slots_.size()) return nullptr;
    Slot& slot = slots_[index];
    if (slot.occupied && slot.generation == uid_gen(uid)) {
        return &slot.obj;
    }
    return nullptr;
}

const ObjectInstance* ObjectManager::get(ObjectUID uid) const {
    uint32_t index = uid_index(uid);
    if (index >= slots_.size()) return nullptr;
    const Slot& slot = slots_[index];
    if (slot.occupied && slot.generation == uid_gen(uid)) {
        return &slot.obj;
    }
    return nullptr;
}

void ObjectManager::move(ObjectUID uid, int nx, int ny, int nz) {
    ObjectInstance* obj = get(uid);
    if (!obj) return;

    int ox = obj->x, oy = obj->y, oz = obj->z;
    spatial_grid_.move(uid, ox, oy, oz, nx, ny, nz);
    obj->x = nx;
    obj->y = ny;
    obj->z = nz;

    event_bus_.announce(EventType::OBJECT_MOVED, {uid, 0, 0.0f});
}

void ObjectManager::tick(Game_map* map) {
    turn_counter_++;

    bool do_medium = (turn_counter_ % 10 == 0);
    bool do_low    = (turn_counter_ % 100 == 0);

    for (auto& slot : slots_) {
        if (!slot.occupied) continue;

        ObjectInstance& obj = slot.obj;
        const auto& proto = proto_db_.get(obj.prototype_id);

        // High frequency: every turn for all non-static objects
        if (proto.tick_freq <= TickFrequency::HIGH && !proto.is_static) {
            tick_high(obj, proto);
        }

        // Medium frequency: every 10 turns for living objects
        if (do_medium && proto.tick_freq <= TickFrequency::MEDIUM && proto.is_living) {
            tick_medium(obj, proto);
        }

        // Low frequency: every 100 turns for everything (decay, growth)
        if (do_low) {
            tick_low(obj, proto);
            // Vegetation update: water absorption from soil, growth, drought
            if (proto.behavior == ObjectBehavior::VEGETATION && obj.vegetation && map) {
                tick_vegetation(obj, proto, map);
            }
        }
    }

    // Clear dirty flags at end of turn
    event_bus_.clear_dirty();
}

void ObjectManager::tick_high(ObjectInstance& obj, const ObjectPrototype& /*proto*/) {
    // High-frequency: AI goal evaluation, movement decisions
    // Currently a stub -- AI systems will hook in via EventBus
    if (obj.life) {
        // If hungry enough, switch goal
        if (obj.life->hunger > 0.8f && obj.life->current_goal == GoalType::IDLE) {
            obj.life->current_goal = GoalType::SEEK_FOOD;
            event_bus_.announce(EventType::LIFE_STATE_CHANGED, {obj.uid, 0, obj.life->hunger});
        }
        // If fatigued enough, switch to sleep
        if (obj.life->fatigue > 0.9f && obj.life->current_goal != GoalType::FLEE) {
            obj.life->current_goal = GoalType::SLEEP;
            event_bus_.announce(EventType::LIFE_STATE_CHANGED, {obj.uid, 0, obj.life->fatigue});
        }
    }
}

void ObjectManager::tick_medium(ObjectInstance& obj, const ObjectPrototype& /*proto*/) {
    // Medium-frequency: update needs
    if (!obj.life) return;

    float old_hunger = obj.life->hunger;

    obj.life->hunger  = std::min(1.0f, obj.life->hunger + obj.life->hunger_rate * 10.0f);
    obj.life->fatigue = std::min(1.0f, obj.life->fatigue + obj.life->fatigue_rate * 10.0f);
    obj.life->morale  = std::max(0.0f, obj.life->morale - obj.life->morale_decay * 10.0f);

    // Starvation damage
    if (obj.life->hunger >= 1.0f) {
        float old_health = obj.health;
        obj.health = std::max(0.0f, obj.health - 1.0f);
        if (obj.health != old_health) {
            event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
        }
    }

    // Announce if needs changed significantly
    if (std::fabs(obj.life->hunger - old_hunger) > 0.05f) {
        event_bus_.announce(EventType::LIFE_STATE_CHANGED, {obj.uid, 0, obj.life->hunger});
    }
}

void ObjectManager::tick_low(ObjectInstance& obj, const ObjectPrototype& proto) {
    // Low-frequency: decay for non-living, non-tree objects
    // Trees have their own tick_tree logic for health management.
    if (!proto.is_living && proto.behavior == ObjectBehavior::INERT && obj.health > 0.0f) {
        // Slow natural decay for items/structures
        float old_health = obj.health;
        obj.health = std::max(0.0f, obj.health - 0.1f);
        if (obj.health != old_health) {
            event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
        }
    }
}

void ObjectManager::tick_vegetation(ObjectInstance& obj, const ObjectPrototype& /*proto*/, Game_map* map) {
    if (!obj.vegetation || !map) return;

    // The tree sits at (x, y, z). The soil voxel is directly below at (x, y, z-1).
    int soil_x = obj.x;
    int soil_y = obj.y;
    int soil_z = obj.z - 1;

    if (!map->is_in_bounds(soil_x, soil_y, soil_z)) return;

    const Tile& soil_tile = map->get_tile(soil_x, soil_y, soil_z);

    // Only absorb water from soil-like materials
    if (soil_tile.material != MaterialType::SOIL_BASE) {
        // No soil beneath -- drought damage
        obj.vegetation->moisture = std::max(0.0f, obj.vegetation->moisture - 0.05f);
        if (obj.vegetation->moisture <= 0.0f) {
            float old_health = obj.health;
            obj.health = std::max(0.0f, obj.health - 2.0f);
            if (obj.health != old_health) {
                event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
            }
        }
        return;
    }

    float soil_water = soil_tile.state.liquid_volume;
    float wilting = soil_tile.wilting_point();

    if (soil_water > wilting) {
        // Absorb water from the soil voxel
        float uptake = std::min(0.02f, soil_water - wilting);

        // Mutate the soil tile's water (const_cast pattern used elsewhere in the
        // codebase for in-place numeric edits on tiles, see climate_hydrology.cpp)
        Tile& mutable_soil = const_cast<Tile&>(map->get_tile(soil_x, soil_y, soil_z));
        mutable_soil.state.liquid_volume -= uptake;

        // Increase tree hydration
        obj.vegetation->moisture = std::min(1.0f, obj.vegetation->moisture + uptake * 5.0f);

        // Growth: advance if well-hydrated
        if (obj.vegetation->moisture > 0.3f && obj.vegetation->growth < 1.0f) {
            obj.vegetation->growth = std::min(1.0f, obj.vegetation->growth + 0.005f);
        }

        // Health regeneration when hydrated
        const auto& tree_proto = proto_db_.get(obj.prototype_id);
        if (obj.vegetation->moisture > 0.2f && obj.health < tree_proto.max_health) {
            obj.health = std::min(tree_proto.max_health, obj.health + 0.5f);
        }
    } else {
        // Soil too dry -- tree loses hydration
        obj.vegetation->moisture = std::max(0.0f, obj.vegetation->moisture - 0.03f);

        // Drought damage when completely dehydrated
        if (obj.vegetation->moisture <= 0.0f) {
            float old_health = obj.health;
            obj.health = std::max(0.0f, obj.health - 1.5f);
            if (obj.health != old_health) {
                event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
            }
        }
    }

    // Fruit cooldown
    if (obj.vegetation->fruit_cooldown > 0) {
        obj.vegetation->fruit_cooldown--;
    }
}

std::vector<ObjectInstance*> ObjectManager::get_all_active() {
    std::vector<ObjectInstance*> result;
    for (auto& slot : slots_) {
        if (slot.occupied) {
            result.push_back(&slot.obj);
        }
    }
    return result;
}
