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

void ObjectManager::kill(ObjectUID uid, Game_map* map) {
    uint32_t index = uid_index(uid);
    if (index >= slots_.size()) return;

    Slot& slot = slots_[index];
    if (!slot.occupied || slot.generation != uid_gen(uid)) return;

    spatial_grid_.remove(uid, slot.obj.x, slot.obj.y, slot.obj.z);

    event_bus_.announce(EventType::OBJECT_KILLED, {uid, 0, 0.0f});

    // Remove any flow blockage the tree was contributing before death
    if (slot.obj.vegetation && map) {
        if (map->is_in_bounds(slot.obj.x, slot.obj.y, slot.obj.z)) {
            map->get_surface(slot.obj.x, slot.obj.y).flow_blockage -= slot.obj.vegetation->current_flow_blockage;
            map->get_surface(slot.obj.x, slot.obj.y).flow_blockage = std::max(0.0f, map->get_surface(slot.obj.x, slot.obj.y).flow_blockage);
        }
    }

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

void ObjectManager::tick(Game_map* map, bool is_world_gen) {
    turn_counter_++;

    bool do_medium = is_world_gen || (turn_counter_ % 10 == 0);
    bool do_low    = is_world_gen || (turn_counter_ % 100 == 0);

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
                tick_vegetation(obj, proto, map, is_world_gen);
            }
        }
    }

    // Garbage Collection: flush the death queue
    for (ObjectUID uid : death_queue_) {
        kill(uid, map);
    }
    death_queue_.clear();

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
            if (obj.health <= 0.0f) death_queue_.push_back(obj.uid);
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
            if (obj.health <= 0.0f) death_queue_.push_back(obj.uid);
        }
    }
}

void ObjectManager::tick_vegetation(ObjectInstance& obj, const ObjectPrototype& /*proto*/, Game_map* map, bool is_world_gen) {
    if (!obj.vegetation || !map) return;

    if (is_world_gen) {
        obj.vegetation->age++;
        obj.vegetation->canopy = std::min(2.0f, 1.0f + obj.vegetation->age * 0.02f);
        obj.vegetation->canopy_density = std::min(1.0f, obj.vegetation->age * 0.01f);
    }

    if (map->is_in_bounds(obj.x, obj.y, obj.z)) {
        const Tile& trunk_tile = map->get_tile(obj.x, obj.y, obj.z);
        float water_level = trunk_tile.state.liquid_volume;

        if (is_world_gen) {
            if (water_level > 0.02f) {
                obj.vegetation->water_damage += water_level;
            } else {
                obj.vegetation->water_damage = std::max(0.0f, obj.vegetation->water_damage - 0.05f);
            }

            float max_water_damage = 1.0f + obj.vegetation->canopy_density * 9.0f;
            if (obj.vegetation->water_damage > max_water_damage || water_level > 1.5f) {
                obj.health = 0.0f;
                event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
                death_queue_.push_back(obj.uid);
                return;
            }
        }

        float new_blockage = std::min(0.8f, obj.vegetation->canopy_density * 0.8f);
        float delta = new_blockage - obj.vegetation->current_flow_blockage;
        map->get_surface(obj.x, obj.y).flow_blockage += delta;
        obj.vegetation->current_flow_blockage = new_blockage;
    }

    if (is_world_gen) {
        // Performance optimization: only compute expensive geometric shade overlaps
        // every 5 years during world generation. Trees still age/grow normally.
        if (obj.vegetation->age % 5 == 0) {
            float total_shade = 0.0f;
            int max_radius = 4; // Max canopy size is 2.0, so overlap distance is max 4.0
            for (int dy = -max_radius; dy <= max_radius; dy++) {
                for (int dx = -max_radius; dx <= max_radius; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = obj.x + dx;
                    int ny = obj.y + dy;
                    if (!map->is_in_bounds(nx, ny, obj.z)) continue;
                    
                    for (ObjectUID other_uid : spatial_grid_.get_at(nx, ny, obj.z)) {
                        ObjectInstance* other = get(other_uid);
                        if (!other || !other->vegetation) continue;
                        
                        float dist_sq = (float)(dx * dx + dy * dy);
                        float overlap_dist = obj.vegetation->canopy + other->vegetation->canopy;

                        if (dist_sq < overlap_dist * overlap_dist) {
                            if (other->vegetation->canopy_density > obj.vegetation->canopy_density || other->vegetation->canopy > obj.vegetation->canopy) {
                                float dist = std::sqrt(dist_sq);
                                float overlap_amount = overlap_dist - dist;
                                float shade_from_tree = std::min(1.5f, overlap_amount * other->vegetation->canopy_density);
                                total_shade += shade_from_tree;
                            }
                        }
                    }
                }
            }

            if (total_shade > 0.0f) {
                obj.vegetation->canopy -= total_shade * 0.2f;
                obj.vegetation->canopy = std::max(0.5f, obj.vegetation->canopy);
            }

            if (total_shade > 2.5f) {
                obj.health = 0.0f;
                event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
                death_queue_.push_back(obj.uid);
                return;
            }
        }
    }

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
                if (obj.health <= 0.0f) death_queue_.push_back(obj.uid);
            }
        }
        return;
    }

    if (!is_world_gen) {
        float soil_water = soil_tile.state.liquid_volume;
        float wilting = soil_tile.wilting_point();
        if (soil_water > wilting) {
            float uptake = std::min(0.02f, soil_water - wilting);
            Tile& mutable_soil = map->get_tile_mut(soil_x, soil_y, soil_z);
            mutable_soil.state.liquid_volume -= uptake;
            obj.vegetation->moisture = std::min(1.0f, obj.vegetation->moisture + uptake * 5.0f);
        } else {
            obj.vegetation->moisture = std::max(0.0f, obj.vegetation->moisture - 0.03f);
        }
    }

    // Apply growth and health based on current moisture (which may have been updated gradually by substeps)
    if (obj.vegetation->moisture > 0.3f && obj.vegetation->growth < 1.0f) {
        obj.vegetation->growth = std::min(1.0f, obj.vegetation->growth + 0.005f);
    }
    const auto& tree_proto = proto_db_.get(obj.prototype_id);
    if (obj.vegetation->moisture > 0.2f && obj.health < tree_proto.max_health) {
        obj.health = std::min(tree_proto.max_health, obj.health + 0.5f);
    }
    if (obj.vegetation->moisture <= 0.0f) {
        float old_health = obj.health;
        obj.health = std::max(0.0f, obj.health - 1.5f);
        if (obj.health != old_health) {
            event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
            if (obj.health <= 0.0f) death_queue_.push_back(obj.uid);
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

void ObjectManager::drink_water_all(Game_map* map, float drink_amount, float dry_amount) {
    if (!map) return;
    for (auto& slot : slots_) {
        if (!slot.occupied) continue;
        ObjectInstance& obj = slot.obj;
        if (obj.vegetation && obj.health > 0.0f) {
            int soil_x = obj.x;
            int soil_y = obj.y;
            int soil_z = obj.z - 1;
            if (map->is_in_bounds(soil_x, soil_y, soil_z)) {
                Tile& t = map->get_tile_mut(soil_x, soil_y, soil_z);
                if (t.material != MaterialType::SOIL_BASE) continue;
                
                float wilting = t.wilting_point();
                if (t.state.liquid_volume > wilting) {
                    float uptake = std::min(drink_amount, t.state.liquid_volume - wilting);
                    t.state.liquid_volume -= uptake;
                    float gain = (uptake / 0.02f) * 0.1f; // Matches the + uptake * 5.0f math
                    obj.vegetation->moisture = std::min(1.0f, obj.vegetation->moisture + gain);
                } else {
                    obj.vegetation->moisture = std::max(0.0f, obj.vegetation->moisture - dry_amount);
                }
            }
        }
    }
}
