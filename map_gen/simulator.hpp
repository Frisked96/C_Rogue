#pragma once

#include "../game_map.hpp"

class MapSimulator {
public:
    static void run(Game_map& game_map, int seed);

private:
    static void simulate_cycle(Game_map& game_map, float water_to_add);
    static void apply_raindrops(Game_map& game_map, float total_water);
    static void simulate_hydrology(Game_map& game_map);
};
