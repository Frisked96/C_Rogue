#include "game_map.hpp"
#include <iostream>
#include <chrono>

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    Game_map map(100, 100, 100);
    map.generate(12345);
    
    int water_tiles = 0;
    for (int z = 0; z < map.get_depth(); ++z) {
        for (int y = 0; y < map.get_height(); ++y) {
            for (int x = 0; x < map.get_width(); ++x) {
                if (map.get_tile(x, y, z).material == MaterialType::WATER_FRESH) {
                    water_tiles++;
                }
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "Map generation took: " << diff.count() << " seconds" << std::endl;
    std::cout << "Water tiles found: " << water_tiles << std::endl;
    
    return 0;
}
