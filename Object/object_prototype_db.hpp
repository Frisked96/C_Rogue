#pragma once
#include "object_prototype.hpp"
#include <cstdint>
#include <vector>

// Central database of all object prototypes.
// Read-only after initialization. Indexed by prototype ID.
class ObjectPrototypeDB {
private:
    std::vector<ObjectPrototype> prototypes_;

public:
    ObjectPrototypeDB();

    // Register a new prototype. Returns its ID.
    uint16_t register_prototype(ObjectPrototype proto);

    // Lookup by ID.
    const ObjectPrototype& get(uint16_t id) const;

    // Total registered count.
    size_t size() const { return prototypes_.size(); }

    // Initialize with built-in prototype definitions.
    void load_defaults();
};
