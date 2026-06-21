#include "object_prototype_db.hpp"
#include <stdexcept>

ObjectPrototypeDB::ObjectPrototypeDB() {
    // Reserve slot 0 as invalid/null prototype
    prototypes_.push_back({
        0, "NULL", '?', 0,
        0.0f, 0.0f, MaterialType::AIR,
        false, true, false, false,
        0, ProfessionType::NONE,
        TickFrequency::LOW
    });
}

uint16_t ObjectPrototypeDB::register_prototype(ObjectPrototype proto) {
    uint16_t id = static_cast<uint16_t>(prototypes_.size());
    proto.id = id;
    prototypes_.push_back(std::move(proto));
    return id;
}

const ObjectPrototype& ObjectPrototypeDB::get(uint16_t id) const {
    if (id >= prototypes_.size()) {
        throw std::out_of_range("ObjectPrototypeDB::get - invalid prototype ID");
    }
    return prototypes_[id];
}

void ObjectPrototypeDB::load_defaults() {
    // --- VEGETATION ---
    register_prototype({
        0, "Oak Tree", 'T', 2,
        500.0f, 200.0f, MaterialType::SOIL_BASE,
        false, true, false, true,
        10, ProfessionType::NONE,
        TickFrequency::LOW
    });

    register_prototype({
        0, "Pine Tree", 'T', 10,
        400.0f, 150.0f, MaterialType::SOIL_BASE,
        false, true, false, true,
        8, ProfessionType::NONE,
        TickFrequency::LOW
    });

    // --- RESOURCES / ROCKS ---
    register_prototype({
        0, "Rock Boulder", 'o', 8,
        1000.0f, 500.0f, MaterialType::STONE_BASE,
        false, true, false, true,
        5, ProfessionType::NONE,
        TickFrequency::LOW
    });

    register_prototype({
        0, "Iron Ore", '*', 7,
        200.0f, 100.0f, MaterialType::STONE_BASE,
        false, true, false, false,
        25, ProfessionType::NONE,
        TickFrequency::LOW
    });

    // --- ITEMS ---
    register_prototype({
        0, "Apple", 'a', 1,
        0.2f, 1.0f, MaterialType::AIR,
        false, false, false, false,
        3, ProfessionType::NONE,
        TickFrequency::LOW
    });

    // --- LIVING: HUMANOIDS ---
    register_prototype({
        0, "Human NPC", 'H', 7,
        75.0f, 100.0f, MaterialType::AIR,
        true, false, false, true,
        0, ProfessionType::FARMER,
        TickFrequency::MEDIUM
    });

    register_prototype({
        0, "Human Corpse", '%', 1,
        75.0f, 0.0f, MaterialType::AIR,
        false, false, true, false,
        0, ProfessionType::NONE,
        TickFrequency::LOW
    });

    // --- LIVING: ANIMALS ---
    register_prototype({
        0, "Wolf", 'w', 8,
        40.0f, 60.0f, MaterialType::AIR,
        true, false, false, true,
        0, ProfessionType::WILD,
        TickFrequency::MEDIUM
    });

    register_prototype({
        0, "Rabbit", 'r', 3,
        2.0f, 10.0f, MaterialType::AIR,
        true, false, false, false,
        0, ProfessionType::WILD,
        TickFrequency::MEDIUM
    });

    // --- CONTAINERS / STRUCTURES ---
    register_prototype({
        0, "Wooden Crate", '#', 6,
        20.0f, 50.0f, MaterialType::AIR,
        false, true, true, true,
        15, ProfessionType::NONE,
        TickFrequency::LOW
    });

    register_prototype({
        0, "Market Stall", 'M', 14,
        100.0f, 80.0f, MaterialType::AIR,
        false, true, true, true,
        50, ProfessionType::MERCHANT,
        TickFrequency::LOW
    });
}
