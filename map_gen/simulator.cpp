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
    int depth = game_map.get_depth();

    // CALIBRATED BUDGET: 0.025m per cell
    float base_annual_water = (float)(width * height) * 0.025f;
    float atmosphere_water = 0.0f; 

    for (int year = 0; year < num_years; ++year) {
        float rainfall = base_annual_water + (atmosphere_water * 0.8f);
        atmosphere_water -= (atmosphere_water * 0.8f);
        apply_raindrops(game_map, rainfall);

        // 1. Vertical Drainage (Very Fast)
        for (int step = 0; step < 10; ++step) {
            atmosphere_water += simulate_hydrology(game_map);
        }

        // 2. Basin Leveling (Once per year - perfectly flat lakes)
        balance_basins(game_map);
        
        atmosphere_water = std::min(atmosphere_water, base_annual_water * 5.0f);
    }
}

void MapSimulator::apply_raindrops(Game_map& game_map, float total_water) {
    int width = game_map.get_width(), height = game_map.get_height(), depth = game_map.get_depth();
    static std::mt19937 drop_gen(42);
    std::uniform_int_distribution<> x_dist(0, width - 1), y_dist(0, height - 1);
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
    int width = game_map.get_width(), height = game_map.get_height(), depth = game_map.get_depth();
    float evap = 0.0f;

    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                if (t.state.moisture <= 0.001f) continue;

                // Simple Evap (0.1% per step)
                float e = t.state.moisture * 0.001f;
                t.state.moisture -= e; evap += e;

                // Vertical Flow Down
                if (z > 0) {
                    Tile& below = const_cast<Tile&>(game_map.get_tile(x, y, z - 1));
                    if (below.material != MaterialType::AIR) {
                        float capacity = (below.material == MaterialType::WATER_FRESH) ? 1.0f : below.effective_porosity();
                        float flow = std::min(t.state.moisture, std::max(0.0f, capacity - below.state.moisture));
                        t.state.moisture -= flow; below.state.moisture += flow;
                    }
                }

                // Pooling
                if (z < depth - 1 && t.state.moisture > 1.0f) {
                    Tile& above = const_cast<Tile&>(game_map.get_tile(x, y, z + 1));
                    if (above.material == MaterialType::AIR) {
                        float pool = t.state.moisture - 1.0f;
                        t.state.moisture = 1.0f;
                        Tile water(MaterialType::WATER_FRESH); water.state.moisture = pool;
                        game_map.set_tile(x, y, z + 1, water);
                    }
                }
            }
        }
    }
    return evap;
}

void MapSimulator::balance_basins(Game_map& game_map) {
    int width = game_map.get_width(), height = game_map.get_height(), depth = game_map.get_depth();
    std::vector<bool> visited(width * height, false);

    for (int z = 0; z < depth; ++z) {
        std::fill(visited.begin(), visited.end(), false);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (visited[y * width + x]) continue;
                Tile& t = const_cast<Tile&>(game_map.get_tile(x, y, z));
                float excess = (t.material == MaterialType::WATER_FRESH) ? t.state.moisture : std::max(0.0f, t.state.moisture - t.effective_porosity());
                if (excess <= 0.01f) continue;

                std::vector<std::pair<int, int>> basin;
                std::queue<std::pair<int, int>> q;
                q.push({x, y}); visited[y * width + x] = true;
                float total_m = 0.0f;

                while (!q.empty()) {
                    auto curr = q.front(); q.pop();
                    const Tile& ct = game_map.get_tile(curr.first, curr.second, z);
                    basin.push_back(curr);
                    total_m += (ct.material == MaterialType::WATER_FRESH) ? ct.state.moisture : (ct.state.moisture - ct.effective_porosity());

                    int dx[] = {1, -1, 0, 0, 1, 1, -1, -1}, dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
                    for (int i = 0; i < 8; ++i) {
                        int nx = curr.first + dx[i], ny = curr.second + dy[i];
                        if (!game_map.is_in_bounds(nx, ny, z) || visited[ny * width + nx]) continue;
                        const Tile& nt = game_map.get_tile(nx, ny, z);
                        bool is_water = (nt.material == MaterialType::WATER_FRESH);
                        bool is_saturated = (nt.state.moisture > nt.effective_porosity() + 0.01f);
                        bool footprint = (z > 0 && game_map.get_tile(nx, ny, z - 1).material != MaterialType::AIR);
                        
                        if (is_water || is_saturated || (footprint && total_m / basin.size() > 0.1f)) {
                            visited[ny * width + nx] = true; q.push({nx, ny});
                        }
                    }
                }

                float avg = total_m / (float)basin.size();
                for (auto& p : basin) {
                    Tile& bt = const_cast<Tile&>(game_map.get_tile(p.first, p.second, z));
                    if (bt.material != MaterialType::WATER_FRESH && avg > 0.05f) {
                        game_map.set_tile(p.first, p.second, z, Tile(MaterialType::WATER_FRESH));
                    }
                    if (bt.material == MaterialType::WATER_FRESH) bt.state.moisture = std::min(1.0f, avg);
                    else bt.state.moisture = bt.effective_porosity() + std::min(1.0f, avg);

                    if (avg > 1.0f && z < depth - 1) {
                        const_cast<Tile&>(game_map.get_tile(p.first, p.second, z + 1)).state.moisture += (avg - 1.0f);
                    }
                }
            }
        }
    }
}
