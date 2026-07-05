#pragma once

#include "../FastNoiseLite.h"
#include "../game_map.hpp"

class ObjectPrototypeDB;
class ObjectManager;

class MapGenerator {
public:
  void generate(Game_map &game_map, int seed = 1337, ObjectPrototypeDB* proto_db = nullptr, ObjectManager* obj_mgr = nullptr);

  // Split steps for diagnostics/benchmarking
  void generate_terrain(Game_map &game_map, int seed);
  void simulate_hydrology(Game_map &game_map, int seed, ObjectPrototypeDB* proto_db = nullptr, ObjectManager* obj_mgr = nullptr);
};