#include "simulator.hpp"
#include <random>
#include <algorithm>
#include <vector>

void MapSimulator::run(Game_map& game_map, int seed) {
    std::mt19937 gen(seed);
    int num_years = 65;

    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();

    // CALIBRATED BUDGET: 0.018m per cell
    float base_annual_water = (float)(width * height) * 0.018f;
    float atmosphere_water = 0.0f; 

    for (int year = 0; year < num_years; ++year) {
        float rainfall = base_annual_water + (atmosphere_water * 0.8f);
        atmosphere_water -= (atmosphere_water * 0.8f);
        apply_raindrops(game_map, rainfall);

        // Run 12 sub-steps per year for absolute consolidation
        for (int step = 0; step < 12; ++step) {
            float evaporated = simulate_hydrology(game_map);
            atmosphere_water += evaporated;
        }
        atmosphere_water = std::min(atmosphere_water, base_annual_water * 5.0f);
    }
}

void MapSimulator::apply_raindrops(Game_map& game_map, float total_water) {
    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();
    static std::mt19937 drop_gen(42);
    std::uniform_int_distribution<> x_dist(0, width - 1);
    std::uniform_int_distribution<> y_dist(0, height - 1);
    std::uniform_real_distribution<float> size_dist(0.01f, 0.05f);
    float water_fallen = 0.0f;
    while (water_fallen < total_water) {
        int rx = x_dist(drop_gen), ry = y_dist(drop_gen);
        float drop_vol = size_dist(drop_gen);
        water_fallen += drop_vol;
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
    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();
    float total_evaporated = 0.0f;

    // 1. Infiltration & Evaporation
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                if (t.material == MaterialType::WATER_FRESH && t.state.moisture < 0.05f) {
                    game_map.set_tile(x, y, z, Tile(MaterialType::AIR));
                    continue;
                }
                bool exposed = (z == depth - 1) || (game_map.get_tile(x, y, z + 1).material == MaterialType::AIR);
                if (exposed && t.state.moisture > 0.001f) {
                    float rate = (t.material == MaterialType::WATER_FRESH) ? 0.01f : 0.005f; // Slower sub-step evap
                    float evap = t.state.moisture * rate;
                    t.state.moisture -= evap;
                    total_evaporated += evap;
                }
                if (t.state.moisture <= 0.001f) continue;
                if (z > 0) {
                    Tile& below = const_cast<Tile&>(game_map.get_tile(x, y, z - 1));
                    if (below.material != MaterialType::AIR && below.material != MaterialType::WATER_FRESH) {
                        float capacity = below.effective_porosity() - below.state.moisture;
                        if (capacity > 0) {
                            float flow = std::min(std::min(t.state.moisture, capacity), t.mat().permeability * 12.0f);
                            t.state.moisture -= flow;
                            below.state.moisture += flow;
                        }
                    }
                }
            }
        }
    }

    // 2. Greedy Lateral Flow & Expansion
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                float excess = (t.material == MaterialType::WATER_FRESH) ? t.state.moisture : std::max(0.0f, t.state.moisture - t.effective_porosity());
                if (excess <= 0.01f) continue;

                int dx[] = {1, -1, 0, 0, 1, 1, -1, -1}, dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
                std::vector<std::pair<int, int>> lower;
                std::vector<std::pair<int, int>> level;
                float current_pot = (float)z + t.state.moisture;

                for (int i = 0; i < 8; ++i) {
                    int nx = x + dx[i], ny = y + dy[i];
                    if (!game_map.is_in_bounds(nx, ny, z)) continue;
                    int nz = z; while (nz > 0 && game_map.get_tile(nx, ny, nz).material == MaterialType::AIR) nz--;
                    const Tile& n = game_map.get_tile(nx, ny, nz);
                    float pot = (float)nz + n.state.moisture;
                    if (pot < current_pot - 0.005f) lower.push_back({nx, ny});
                    else if (std::abs(pot - current_pot) < 0.01f) level.push_back({nx, ny});
                }

                if (!lower.empty()) {
                    float min_p = 999.0f; int bx = x, by = y;
                    for (auto& ln : lower) {
                        int nz = z; while (nz > 0 && game_map.get_tile(ln.first, ln.second, nz).material == MaterialType::AIR) nz--;
                        float p = (float)nz + game_map.get_tile(ln.first, ln.second, nz).state.moisture;
                        if (p < min_p) { min_p = p; bx = ln.first; by = ln.second; }
                    }
                    int tz = z; while (tz > 0 && game_map.get_tile(bx, by, tz).material == MaterialType::AIR) tz--;
                    Tile& target = const_cast<Tile&>(game_map.get_tile(bx, by, tz));
                    float flow = excess * 0.95f; 
                    t.state.moisture -= flow; target.state.moisture += flow;
                    // EROSION
                    if (flow > 0.02f && t.mat().erodibility > 0.0f) {
                        float erosion = std::min(t.state.structural_integrity, flow * (current_pot - min_p) * t.mat().erodibility * 0.05f); // LIGHT
                        t.state.structural_integrity -= erosion;
                        if (t.state.structural_integrity < 0.15f && t.material != MaterialType::STONE_BASE) game_map.set_tile(x, y, z, Tile(MaterialType::AIR));
                    }
                } else if (!level.empty()) {
                    for (auto& ln : level) {
                        int tz = z; while (tz > 0 && game_map.get_tile(ln.first, ln.second, tz).material == MaterialType::AIR) tz--;
                        Tile& target = const_cast<Tile&>(game_map.get_tile(ln.first, ln.second, tz));
                        if (target.state.moisture < t.state.moisture) {
                            float flow = (t.state.moisture - target.state.moisture) * 0.5f;
                            t.state.moisture -= flow; target.state.moisture += flow;
                        }
                    }
                }
            }
        }
    }

    // 3. Pooling
    for (int z = 0; z < depth - 1; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                if (t.material == MaterialType::AIR || t.material == MaterialType::WATER_FRESH) continue;
                float excess = t.state.moisture - t.effective_porosity();
                if (excess > 0.15f) { // LOWER THRESHOLD FOR BASINS
                    Tile& above = const_cast<Tile&>(game_map.get_tile(x, y, z + 1));
                    if (above.material == MaterialType::AIR) {
                        float pool_vol = excess; 
                        t.state.moisture -= pool_vol;
                        Tile water_tile(MaterialType::WATER_FRESH);
                        water_tile.state.moisture = pool_vol;
                        game_map.set_tile(x, y, z + 1, water_tile);
                    }
                }
            }
        }
    }
    return total_evaporated;
}
