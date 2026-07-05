#pragma once
#include "object_manager.hpp"
#include "object_prototype_db.hpp"

class Game_map;

// ObjectSpawner: A standalone system that populates the world with objects
// during map generation. It reads SpawnRequirements from each prototype
// and places objects where conditions are met.
//
// The spawner is NOT part of ObjectManager -- objects are passive data.
// This system reads their requirements and acts on them.
namespace ObjectSpawner {

// Populate the map with objects based on prototype SpawnRequirements.
// Should be called AFTER terrain generation and hydrology simulation,
// so that soil moisture and temperature data are available.
//
// ground_z: precomputed heightmap (index = y * width + x) giving the
//           topmost solid z for each column. If empty, it will be
//           computed internally.
void populate(Game_map& map, ObjectPrototypeDB& proto_db,
              ObjectManager& obj_mgr, int seed,
              const std::vector<int>& ground_z = {});

}  // namespace ObjectSpawner
