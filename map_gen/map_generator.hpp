#pragma once

#include "../game_map.hpp"
#include "../FastNoiseLite.h"

class MapGenerator {
public:
    static void generate(Game_map& game_map, int seed = 1337);

private:
    static MaterialType get_material_for_height(int z, int max_z, float noise_val);
};
