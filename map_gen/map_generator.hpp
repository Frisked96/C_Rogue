#pragma once

#include "../FastNoiseLite.h"
#include "../game_map.hpp"

class MapGenerator {
public:
  void generate(Game_map &game_map, int seed = 1337);

  // Split steps for diagnostics/benchmarking
  void generate_terrain(Game_map &game_map, int seed);
  void simulate_hydrology(Game_map &game_map, int seed);
};