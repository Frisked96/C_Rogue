#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <queue>
#include <set>
#include "game_map.hpp"
#include "map_gen/map_generator.hpp"
#include "map_gen/simulator.hpp"

struct Stats {
    double median;
    double stddev;
};

Stats calculate_stats(std::vector<int>& elevations) {
    if (elevations.empty()) return {0, 0};
    std::sort(elevations.begin(), elevations.end());
    double median = elevations[elevations.size() / 2];
    double sum = 0;
    for (int e : elevations) sum += e;
    double mean = sum / elevations.size();
    double sq_sum = 0;
    for (int e : elevations) sq_sum += (e - mean) * (e - mean);
    double stddev = std::sqrt(sq_sum / elevations.size());
    return {median, stddev};
}

void analyze_map(const Game_map& map, const std::string& label) {
    int w = map.get_width();
    int h = map.get_height();
    int d = map.get_depth();

    int water_tiles = 0;
    int surface_water_cols = 0;
    std::vector<int> full_elevation(w * h);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            bool has_water = false;
            int surface_z = 0;
            for (int z = d - 1; z >= 0; --z) {
                const Tile& t = map.get_tile(x, y, z);
                if (t.material == MaterialType::WATER_FRESH) {
                    water_tiles++;
                    has_water = true;
                }
                if (surface_z == 0 && t.material != MaterialType::AIR) {
                    surface_z = z;
                }
            }
            if (has_water) surface_water_cols++;
            full_elevation[y * w + x] = surface_z;
        }
    }

    std::cout << "--- " << label << " ---" << std::endl;
    std::cout << "Surface Coverage: " << (surface_water_cols * 100.0 / (w * h)) << "% (" << surface_water_cols << "/10000 cols)" << std::endl;
    std::cout << "Total Water Tiles: " << water_tiles << std::endl;
    
    // Sample Stats
    for (int i = 0; i < 3; ++i) {
        int sx = (i * 30) % (w - 10);
        int sy = (i * 40) % (h - 10);
        std::vector<int> block;
        for (int y = sy; y < sy + 10; ++y)
            for (int x = sx; x < sx + 10; ++x)
                block.push_back(full_elevation[y * w + x]);
        Stats s = calculate_stats(block);
        std::cout << "Block " << i << "@(" << sx << "," << sy << "): Median=" << s.median << ", StdDev=" << s.stddev << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    Game_map map(100, 100, 100);
    std::cout << "1. Base..." << std::endl;
    MapGenerator::generate(map, 42); 
    analyze_map(map, "Post-Simulation");
    return 0;
}
