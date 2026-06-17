#pragma once

#include "../FastNoiseLite.h"
#include "../game_map.hpp"

class MapGenerator {
public:
  static void generate(Game_map &game_map, int seed = 1337);
};