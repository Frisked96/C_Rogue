#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include "game_map.hpp"
#include "map_gen/map_generator.hpp"
#include "map_gen/simulator.hpp"
#include "tile.hpp"

int main(int argc, char* argv[]) {
    int width = 80;
    int height = 80;
    int depth = 40;
    int seed = 1337;

    if (argc > 1) width = std::stoi(argv[1]);
    if (argc > 2) height = std::stoi(argv[2]);
    if (argc > 3) seed = std::stoi(argv[3]);

    Game_map map(width, height, depth);

    auto start_total = std::chrono::high_resolution_clock::now();
    
    // We need to instrument the generator to get the simulator instance or 
    // run it manually here. Let's run it manually to get the timings.
    
    // 1. Initial Terrain Generation (from map_generator.cpp logic)
    auto start_gen = std::chrono::high_resolution_clock::now();
    MapGenerator gen;
    // We'll reimplement the core of MapGenerator::generate here to access the simulator
    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetFrequency(0.005f);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float noise_val = noise.GetNoise((float)x, (float)y);
            int terrain_height = static_cast<int>((noise_val + 1.0f) * 0.5f * (depth * 0.7f)) + 5;
            for (int z = 0; z < depth; z++) {
                if (z < terrain_height) {
                    if (z > terrain_height - 3) {
                        map.set_tile(x, y, z, Tile(MaterialType::SOIL_BASE));
                    } else {
                        map.set_tile(x, y, z, Tile(MaterialType::STONE_BASE));
                    }
                } else {
                    map.set_tile(x, y, z, Tile(MaterialType::AIR));
                }
            }
        }
    }
    auto end_gen = std::chrono::high_resolution_clock::now();
    double gen_time = std::chrono::duration<double>(end_gen - start_gen).count();

    // 2. Hydrology Simulation
    MapSimulator simulator;
    auto start_sim = std::chrono::high_resolution_clock::now();
    simulator.run(map, seed);
    auto end_sim = std::chrono::high_resolution_clock::now();
    double sim_time = std::chrono::duration<double>(end_sim - start_sim).count();

    auto end_total = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double>(end_total - start_total).count();

    // Stats gathering
    long water_fresh_tiles = 0;
    long tiles_with_water_in_name = 0;
    long tiles_with_moisture = 0;
    double total_moisture = 0.0;
    int max_z_water_moisture = -1;
    int min_z_water_moisture = depth;

    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Tile& t = map.get_tile(x, y, z);
                bool has_water_material = (t.material == MaterialType::WATER_FRESH);
                bool name_has_water = (t.mat().name.find("Water") != std::string::npos || t.mat().name.find("water") != std::string::npos);
                bool has_moisture = (t.state.moisture > 1e-6f);

                if (has_water_material) water_fresh_tiles++;
                if (name_has_water) tiles_with_water_in_name++;
                if (has_moisture) {
                    tiles_with_moisture++;
                    total_moisture += t.state.moisture;
                }

                if (has_water_material || has_moisture) {
                    if (z > max_z_water_moisture) max_z_water_moisture = z;
                    if (z < min_z_water_moisture) min_z_water_moisture = z;
                }
            }
        }
    }

    // Output for Python
    std::cout << "METRIC_START" << std::endl;
    std::cout << "total_time: " << total_time << std::endl;
    std::cout << "terrain_gen_time: " << gen_time << std::endl;
    std::cout << "hydrology_sim_time: " << sim_time << std::endl;
    
    const auto& timings = simulator.get_timings();
    for (const auto& pair : timings) {
        std::cout << "sim_step_" << pair.first << ": " << pair.second << std::endl;
    }

    std::cout << "water_fresh_tiles: " << water_fresh_tiles << std::endl;
    std::cout << "tiles_with_water_in_name: " << tiles_with_water_in_name << std::endl;
    std::cout << "tiles_with_moisture: " << tiles_with_moisture << std::endl;
    std::cout << "total_moisture: " << total_moisture << std::endl;
    std::cout << "max_z_water_moisture: " << max_z_water_moisture << std::endl;
    std::cout << "min_z_water_moisture: " << min_z_water_moisture << std::endl;
    std::cout << "METRIC_END" << std::endl;

    return 0;
}
