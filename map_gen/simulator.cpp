// Thermodynamic Hydrology Simulator
#include "simulator.hpp"
#include <random>
#include <algorithm>
#include <vector>
#include <queue>

void MapSimulator::run(Game_map& game_map, int seed) {
    std::mt19937 gen(seed);
    int num_years = 65;
    int width = game_map.get_width();
    int height = game_map.get_height();

    // Atmosphere starts with some base humidity
    float atmosphere_water = (float)(width * height) * 0.01f; 
    std::uniform_real_distribution<float> rain_dist(0.01f, 0.05f);

    for (int year = 0; year < num_years; ++year) {
        // Stochastic Rainfall: Base rain + 20% of carried-over humidity
        float rain_budget = (float)(width * height) * rain_dist(gen);
        float total_rain = rain_budget + (atmosphere_water * 0.2f);
        atmosphere_water -= (atmosphere_water * 0.2f);
        
        apply_raindrops(game_map, total_rain);

        // Run flux simulation (15 steps per year for thermodynamic stability)
        for (int step = 0; step < 15; ++step) {
            atmosphere_water += simulate_hydrology(game_map);
        }
        
        // Clamp global humidity to prevent infinite cycles
        atmosphere_water = std::min(atmosphere_water, (float)(width * height) * 2.0f);
    }
}

void MapSimulator::apply_raindrops(Game_map& game_map, float total_water) {
    int width = game_map.get_width(), height = game_map.get_height(), depth = game_map.get_depth();
    static std::mt19937 drop_gen(42);
    std::uniform_int_distribution<> x_dist(0, width - 1), y_dist(0, height - 1);
    std::uniform_real_distribution<float> size_dist(0.01f, 0.08f);

    float water_fallen = 0.0f;
    while (water_fallen < total_water) {
        int rx = x_dist(drop_gen), ry = y_dist(drop_gen);
        float drop_vol = size_dist(drop_gen);
        water_fallen += drop_vol;

        // Rain hits the first non-air tile from the top
        for (int rz = depth - 1; rz >= 0; --rz) {
            Tile& t = const_cast<Tile&>(game_map.get_tile(rx, ry, rz));
            if (t.material != MaterialType::AIR) {
                t.state.moisture += drop_vol;
                break;
            }
        }
    }
}

float MapSimulator::simulate_hydrology(Game_map& game_map) {
    int width = game_map.get_width(), height = game_map.get_height(), depth = game_map.get_depth();
    float total_evap = 0.0f;

    // Optimization: find max terrain height to skip empty sky (hardcoded limit for now based on generation)
    const int MAX_SIM_Z = 75; 

    // Iterate top-down to simulate gravity properly
    for (int z = MAX_SIM_Z; z >= 0; --z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                
                // --- 1. Edge Runoff (Aggressive Drain) ---
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    if (t.state.moisture > 0.001f) {
                        float runoff = t.state.moisture * 0.5f; // 50% drain per step at edges
                        t.state.moisture -= runoff;
                    }
                }

                // Optimization: Skip empty air and dry tiles
                if (t.state.moisture <= 0.0001f && t.material == MaterialType::AIR) continue;

                // --- 2. Thermodynamic Evaporation ---
                bool is_surface = (z == depth - 1) || (game_map.get_tile(x, y, z + 1).material == MaterialType::AIR);
                if (is_surface && t.state.moisture > 0) {
                    float temp_factor = std::max(0.1f, (t.state.temperature - 273.15f) / 15.0f);
                    float evap = t.state.moisture * 0.02f * temp_factor; // Increased evap
                    t.state.moisture -= evap;
                    total_evap += evap;
                }

                if (t.state.moisture <= 0.0001f) {
                    // Cleanup dry water tiles
                    if (t.material == MaterialType::WATER_FRESH) {
                        Tile air_tile(MaterialType::AIR);
                        air_tile.state = t.state;
                        game_map.set_tile(x, y, z, air_tile);
                    }
                    continue;
                }

                // --- 3. Vertical Infiltration (Gravity) ---
                if (z > 0) {
                    Tile& below = const_cast<Tile&>(game_map.get_tile(x, y, z - 1));
                    float capacity = (below.material == MaterialType::AIR || below.material == MaterialType::WATER_FRESH) ? 1.0f : below.effective_porosity();
                    
                    float k = t.mat().permeability;
                    float flow = std::min(t.state.moisture, (capacity - below.state.moisture) * k * 100.0f);
                    
                    if (flow > 0.0001f) {
                        t.state.moisture -= flow;
                        below.state.moisture += flow;
                        if (below.material == MaterialType::AIR && below.state.moisture > 0.05f) {
                            Tile water_tile(MaterialType::WATER_FRESH);
                            water_tile.state = below.state;
                            game_map.set_tile(x, y, z - 1, water_tile);
                        }
                    }
                }

                // --- 4. Subsurface & Lateral Pressure Flow ---
                float flow_threshold = (t.material == MaterialType::WATER_FRESH) ? 0.01f : (t.effective_porosity() * 0.2f);
                if (t.state.moisture > flow_threshold) {
                    int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
                    for (int i = 0; i < 4; ++i) {
                        int nx = x + dx[i], ny = y + dy[i];
                        if (!game_map.is_in_bounds(nx, ny, z)) continue;

                        Tile& nt = const_cast<Tile&>(game_map.get_tile(nx, ny, z));
                        float nt_cap = (nt.material == MaterialType::WATER_FRESH || nt.material == MaterialType::AIR) ? 1.0f : nt.effective_porosity();

                        float k_avg = (t.mat().permeability + nt.mat().permeability) * 0.5f;
                        float flow_mult = (t.material == MaterialType::WATER_FRESH || nt.material == MaterialType::WATER_FRESH) ? 0.2f : (k_avg * 50.0f);

                        if (z > 0 && game_map.get_tile(nx, ny, z - 1).material == MaterialType::AIR) {
                            flow_mult *= 1.5f;
                        }

                        if (nt.state.moisture < t.state.moisture) {
                            float pressure_flow = (t.state.moisture - nt.state.moisture) * flow_mult;
                            t.state.moisture -= pressure_flow;
                            nt.state.moisture += pressure_flow;

                            if (nt.material == MaterialType::AIR && nt.state.moisture > 0.1f) {
                                Tile water_tile(MaterialType::WATER_FRESH);
                                water_tile.state = nt.state;
                                game_map.set_tile(nx, ny, z, water_tile);
                            }
                        }
                    }
                }

                // --- 5. Upward Saturation (Pooling) ---
                float max_vol = (t.material == MaterialType::WATER_FRESH) ? 1.0f : t.effective_porosity();
                if (t.state.moisture > max_vol && z < depth - 1) {
                    Tile& above = const_cast<Tile&>(game_map.get_tile(x, y, z + 1));
                    float overflow = t.state.moisture - max_vol;
                    t.state.moisture = max_vol;
                    above.state.moisture += overflow;
                    if (above.material == MaterialType::AIR && above.state.moisture > 0.05f) {
                        Tile water_tile(MaterialType::WATER_FRESH);
                        water_tile.state = above.state;
                        game_map.set_tile(x, y, z + 1, water_tile);
                    }
                }
            }
        }
    }
    return total_evap;
}

// Dummy implementation since balance_basins is removed from the loop but still in header
float MapSimulator::balance_basins(Game_map& game_map) { 
    (void)game_map; 
    return 0; 
}
