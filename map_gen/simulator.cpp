#include "simulator.hpp"
#include <random>
#include <algorithm>
#include <vector>

void MapSimulator::run(Game_map& game_map, int seed) {
    std::mt19937 gen(seed);
    int num_years = 50;

    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();

    // Total water budget for the whole "history"
    float base_water_volume = (float)(width * height * depth) * 0.005f;
    float water_per_year = base_water_volume / num_years;

    for (int year = 0; year < num_years; ++year) {
        // Season 1: Spring (Heavy Rain)
        apply_raindrops(game_map, water_per_year * 1.5f);
        simulate_hydrology(game_map);

        // Season 2: Summer (Dry / Evaporation)
        // (Evaporation logic integrated into simulate_hydrology or separate)
        
        // Season 3: Autumn (Moderate Rain)
        apply_raindrops(game_map, water_per_year * 0.8f);
        simulate_hydrology(game_map);
        
        // Season 4: Winter (Low rain, maybe nutrient settling)
    }
}

void MapSimulator::apply_raindrops(Game_map& game_map, float total_water) {
    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();

    float raindrop_vol = 0.05f; 
    int num_drops = static_cast<int>(total_water / raindrop_vol);

    static std::mt19937 drop_gen(42);
    std::uniform_int_distribution<> x_dist(0, width - 1);
    std::uniform_int_distribution<> y_dist(0, height - 1);

    for (int i = 0; i < num_drops; ++i) {
        int rx = x_dist(drop_gen);
        int ry = y_dist(drop_gen);

        for (int rz = depth - 1; rz >= 0; --rz) {
            Tile& t = const_cast<Tile&>(game_map.get_tile(rx, ry, rz));
            if (t.material != MaterialType::AIR) {
                t.state.moisture += raindrop_vol;
                break;
            }
        }
    }
}

void MapSimulator::simulate_hydrology(Game_map& game_map) {
    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();

    // 1. Infiltration & Erosion
    for (int z = 1; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                if (t.state.moisture <= 0) continue;

                Tile& below = const_cast<Tile&>(game_map.get_tile(x, y, z - 1));
                
                // Infiltration
                if (below.material != MaterialType::AIR && below.material != MaterialType::WATER_FRESH) {
                    float capacity = below.effective_porosity() - below.state.moisture;
                    if (capacity > 0) {
                        float flow = std::min({t.state.moisture, capacity, t.mat().permeability * 5.0f});
                        t.state.moisture -= flow;
                        below.state.moisture += flow;
                        below.state.nutrients_N = std::min(1.0f, below.state.nutrients_N + flow * 0.05f);
                    }
                }
            }
        }
    }

    // 2. Lateral Runoff and Soil Erosion/Deposition
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                float excess = t.state.moisture - t.effective_porosity();
                if (excess <= 0) continue;

                int dx[] = {0, 0, 1, -1};
                int dy[] = {1, -1, 0, 0};
                
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (!game_map.is_in_bounds(nx, ny, z)) continue;

                    Tile& n = const_cast<Tile&>(game_map.get_tile(nx, ny, z));
                    
                    // Simple runoff to neighbors that are lower or have less water
                    // (For simplicity, we just check if neighbor is not a wall)
                    if (n.material == MaterialType::AIR || n.material == MaterialType::WATER_FRESH || n.state.moisture < t.state.moisture) {
                        float flow = std::min(excess, (t.state.moisture - n.state.moisture) * 0.5f);
                        if (flow <= 0) continue;

                        t.state.moisture -= flow;
                        n.state.moisture += flow;
                        excess -= flow;

                        // GRADUAL EROSION: High flow carries sediment
                        if (flow > 0.05f && t.mat().erodibility > 0.0f) {
                            float erosion_amount = std::min(t.state.structural_integrity, flow * t.mat().erodibility * 0.1f);
                            t.state.structural_integrity -= erosion_amount;
                            t.state.sediment += erosion_amount;

                            // Transfer sediment and nutrients to neighbor
                            float transferred_sediment = t.state.sediment * (flow / t.state.moisture);
                            float transferred_nutrients = t.state.nutrients_N * (flow / t.state.moisture);
                            
                            t.state.sediment -= transferred_sediment;
                            n.state.sediment += transferred_sediment;
                            
                            t.state.nutrients_N -= transferred_nutrients;
                            n.state.nutrients_N += transferred_nutrients;

                            // DEPOSITION: If neighbor has low flow or is a pool, deposit sediment
                            if (flow < 0.1f && n.state.sediment > 0.05f) {
                                float deposit = std::min(0.05f, n.state.sediment);
                                n.state.structural_integrity = std::min(1.0f, n.state.structural_integrity + deposit);
                                n.state.sediment -= deposit;
                            }

                            // If tile is fully eroded, it becomes AIR
                            if (t.state.structural_integrity <= 0.01f && t.material != MaterialType::STONE_BASE) {
                                game_map.set_tile(x, y, z, Tile(MaterialType::AIR));
                            }
                        }

                        if (excess <= 0) break;
                    }
                }

                // POOLING: If still excess, spill up into AIR/WATER
                if (excess > 0.1f && z < depth - 1) {
                    Tile& above = const_cast<Tile&>(game_map.get_tile(x, y, z + 1));
                    if (above.material == MaterialType::AIR) {
                        game_map.set_tile(x, y, z + 1, Tile(MaterialType::WATER_FRESH));
                        t.state.moisture -= 0.1f;
                    } else if (above.material == MaterialType::WATER_FRESH) {
                        // Already water, just let it exist
                    }
                }
            }
        }
    }
}
