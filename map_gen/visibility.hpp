#pragma once
#include "../game_map.hpp"

class Visibility {
public:
    static void compute_fov(Game_map& map, int start_x, int start_y, int start_z, int radius);
};
