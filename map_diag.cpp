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
    if (argc > 4) depth = std::stoi(argv[4]);

    Game_map map(width, height, depth);
    MapGenerator generator;

    auto start_total = std::chrono::high_resolution_clock::now();
    
    // 1. Initial Terrain Generation
    auto start_gen = std::chrono::high_resolution_clock::now();
    generator.generate_terrain(map, seed);
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

    // Stats gathering - Comprehensive Map Data Check
    long water_surface_tiles = 0; 
    long deep_water_tiles = 0;
    long shallow_water_tiles = 0;
    long soil_tiles_total = 0;
    long soil_tiles_with_moisture = 0;
    long frozen_tiles = 0;        
    
    double total_surface_water_vol = 0.0;
    double total_soil_moisture_vol = 0.0;
    double total_ice_vol = 0.0;

    int max_z_surface_water = -1;
    int min_z_liquid = depth;
    int max_z_soil = -1;

    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Tile& t = map.get_tile(x, y, z);
                
                // Track highest soil peak
                if (t.material == MaterialType::SOIL_BASE) {
                    soil_tiles_total++;
                    if (z > max_z_soil) max_z_soil = z;
                    
                    if (t.state.liquid_volume > 1e-6f) {
                        soil_tiles_with_moisture++;
                        total_soil_moisture_vol += t.state.liquid_volume;
                        if (z < min_z_liquid) min_z_liquid = z;
                    }
                } else if (t.material == MaterialType::STONE_BASE) {
                    if (t.state.liquid_volume > 1e-6f) {
                        total_soil_moisture_vol += t.state.liquid_volume;
                        if (z < min_z_liquid) min_z_liquid = z;
                    }
                }

                // 1. Surface Water (Liquids in AIR - physical lakes/rivers)
                if (t.material == MaterialType::AIR && t.state.liquid_volume > 1e-5f) {
                    water_surface_tiles++;
                    if (t.state.liquid_volume >= 0.4f) deep_water_tiles++;
                    else shallow_water_tiles++;

                    total_surface_water_vol += t.state.liquid_volume;
                    if (z > max_z_surface_water) max_z_surface_water = z;
                    if (z < min_z_liquid) min_z_liquid = z;
                }
                
                // 3. Ice/Snow
                if (t.state.frozen_volume > 1e-6f) {
                    frozen_tiles++;
                    total_ice_vol += t.state.frozen_volume;
                }
            }
        }
    }

    // Output for Python parsing
    std::cout << "METRIC_START" << std::endl;
    std::cout << "total_time: " << total_time << std::endl;
    std::cout << "terrain_gen_time: " << gen_time << std::endl;
    std::cout << "hydrology_sim_time: " << sim_time << std::endl;
    
    const auto& timings = simulator.get_timings();
    for (const auto& pair : timings) {
        std::cout << "sim_step_" << pair.first << ": " << pair.second << std::endl;
    }

    std::cout << "water_surface_tiles: " << water_surface_tiles << std::endl;
    std::cout << "deep_water_tiles: " << deep_water_tiles << std::endl;
    std::cout << "shallow_water_tiles: " << shallow_water_tiles << std::endl;
    std::cout << "soil_tiles_total: " << soil_tiles_total << std::endl;
    std::cout << "soil_tiles_with_moisture: " << soil_tiles_with_moisture << std::endl;
    std::cout << "frozen_tiles: " << frozen_tiles << std::endl;
    std::cout << "total_surface_water_vol: " << total_surface_water_vol << std::endl;
    std::cout << "total_soil_moisture_vol: " << total_soil_moisture_vol << std::endl;
    std::cout << "total_ice_vol: " << total_ice_vol << std::endl;
    std::cout << "max_z_surface_water: " << max_z_surface_water << std::endl;
    std::cout << "min_z_liquid: " << min_z_liquid << std::endl;
    std::cout << "max_z_soil: " << max_z_soil << std::endl;
    std::cout << "METRIC_END" << std::endl;

    return 0;
}
