// simulator.hpp
#pragma once

#include "../game_map.hpp"

class MapSimulator {
public:
    static void run(Game_map& game_map, int seed);

private:
    static void apply_raindrops(Game_map& game_map, float total_water);
    static float simulate_hydrology(Game_map& game_map);
    static float balance_basins(Game_map& game_map);
};