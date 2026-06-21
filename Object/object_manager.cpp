#include "object_manager.hpp"
#include <algorithm>
#include <cmath>

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

void ObjectManager::tick() {
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
    // Low-frequency: decay for non-living objects, vegetation growth
    if (!proto.is_living && obj.health > 0.0f) {
        // Slow natural decay for items/structures
        float old_health = obj.health;
        obj.health = std::max(0.0f, obj.health - 0.1f);
        if (obj.health != old_health) {
            event_bus_.announce(EventType::HEALTH_CHANGED, {obj.uid, 0, obj.health});
        }
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
