#include "game_map.hpp"
#include <iostream>
#include <chrono>

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    Game_map map(100, 100, 100);
    map.generate(12345);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "Map generation took: " << diff.count() << " seconds" << std::endl;
    
    return 0;
}
