#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "../game_map.hpp"
#include "../map_gen/simulator.hpp"
#include "../FastNoiseLite.h"
#include "../Object/object_prototype_db.hpp"
#include "../Object/object_manager.hpp"
#include "../Object/event_bus.hpp"
#include "../Object/object_spawner.hpp"

// --- Configuration Constants ---
constexpr int MAP_WIDTH = 200;
constexpr int MAP_HEIGHT = 100;
constexpr int MAP_DEPTH = 150;
constexpr int SEED = 12345;
constexpr int SPAWN_INTERVAL = 1; // Spawn trees frequently for the test
constexpr int MAX_SPAWN_YEAR = 100;

constexpr int VIEW_START_X = 35;
constexpr int VIEW_START_Y = 35;
constexpr int VIEW_WIDTH = 100;
constexpr int VIEW_HEIGHT = 40;

// --- Custom Test Tree Logic ---
struct TestTree {
    int x, y, z;
    int age = 0;
    float canopy = 1.0f;
    float canopy_density = 0.1f;
    float water_damage = 0.0f;
    int max_age = 0;
};

std::unordered_map<ObjectUID, TestTree> custom_trees;
int global_trees_died_water = 0;
int global_trees_died_shade = 0;

void print_map_slice(const Game_map& map, ObjectManager& obj_mgr, int start_x, int start_y, int w, int h) {
    std::string out;
    for (int y = start_y; y < start_y + h; ++y) {
        for (int x = start_x; x < start_x + w; ++x) {
            // Find surface
            int z = map.get_depth() - 1;
            while (z > 0 && map.get_tile(x, y, z).material == MaterialType::AIR && map.get_tile(x, y, z).state.liquid_volume <= 0.0f && map.get_tile(x, y, z).state.frozen_volume <= 0.0f) {
                z--;
            }
            
            // Check for objects first
            char glyph = ' ';
            int fg = 7;
            int bg = 0; // 0 means default background
            bool found_obj = false;
            
            // Look for objects at z+1 or z
            for (int obj_z = z + 1; obj_z >= z; --obj_z) {
                if (obj_mgr.spatial().has_any(x, y, obj_z)) {
                    for (auto uid : obj_mgr.spatial().get_at(x, y, obj_z)) {
                        auto* obj = obj_mgr.get(uid);
                        if (obj) {
                            const auto& proto = obj_mgr.proto_db().get(obj->prototype_id);
                            glyph = proto.glyph;
                            fg = proto.fg_color;
                            
                            // Custom test logic for growth stages
                            if (proto.id == 1 && custom_trees.count(uid)) {
                                if (custom_trees[uid].age < 15 || custom_trees[uid].canopy < 1.9f) {
                                    glyph = 't'; // Young tree
                                } else {
                                    glyph = 'T'; // Mature tree
                                }
                            }
                            
                            // Visualizer: If tree is standing in water, give it a blue background
                            if (map.get_tile(x, y, obj_z).state.liquid_volume > 0.01f) {
                                bg = 44; // ANSI Blue background
                            }
                            
                            found_obj = true;
                            break;
                        }
                    }
                }
                if (found_obj) break;
            }
            
            if (!found_obj) {
                const auto& tile = map.get_tile(x, y, z);
                if (tile.material == MaterialType::AIR && (tile.state.liquid_volume > 0.0f || tile.state.frozen_volume > 0.0f)) {
                    glyph = '~';
                    fg = 33; // Blue
                } else {
                    glyph = tile.get_glyph();
                    fg = tile.mat().fg_color;
                }
            }
            
            if (bg != 0) {
                out += "\033[38;5;" + std::to_string(fg) + ";48;5;" + std::to_string(bg) + "m" + glyph + "\033[0m"; // Reset to stop bleeding
            } else {
                out += "\033[38;5;" + std::to_string(fg) + "m" + glyph;
            }
        }
        out += "\033[0m\n";
    }
    std::cout << out;
}

void test_populate(Game_map& map, ObjectManager& obj_mgr, int year, const std::vector<int>& ground_z) {
    if (year > MAX_SPAWN_YEAR) return;
    if (year % SPAWN_INTERVAL != 0) return;
    
    srand(SEED + year);
    uint16_t proto_id = 1; // Oak Tree
    
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            int gz = ground_z[y * MAP_WIDTH + x];
            
            if (gz < 15 || gz > 60) continue;
            
            int spawn_z = gz + 1;
            if (spawn_z >= MAP_DEPTH) continue;
            const Tile& trunk_tile = map.get_tile(x, y, spawn_z);
            if (trunk_tile.material != MaterialType::AIR) continue;
            if (trunk_tile.state.liquid_volume > 0.05f) continue; 
            
            const Tile& ground = map.get_tile(x, y, gz);
            if (ground.material != MaterialType::SOIL_BASE) continue;
            
            float moisture = ground.state.liquid_volume;
            if (moisture < 0.08f || moisture > 0.6f) continue; 
            
            float prob = 0.02f;
            if (moisture > 0.25f && moisture < 0.45f) prob = 0.08f; 
            
            if ((float)rand() / RAND_MAX > prob) continue;
            if (obj_mgr.spatial().has_any(x, y, spawn_z)) continue;
            
            ObjectUID uid = obj_mgr.spawn(proto_id, x, y, spawn_z);
            TestTree t;
            t.x = x; t.y = y; t.z = spawn_z;
            t.max_age = 40 + (rand() % 40); 
            custom_trees[uid] = t;
        }
    }
}

void test_tick_trees(Game_map& map, ObjectManager& obj_mgr) {
    std::vector<ObjectUID> to_kill_water;
    std::vector<ObjectUID> to_kill_shade;
    
    for (auto& pair : custom_trees) {
        ObjectUID uid = pair.first;
        TestTree& t = pair.second;
        
        t.age++;
        t.canopy = std::min(2.0f, 1.0f + t.age * 0.02f);
        t.canopy_density = std::min(1.0f, t.age * 0.01f);
        
        const Tile& trunk_tile = map.get_tile(t.x, t.y, t.z);
        float water_level = trunk_tile.state.liquid_volume;
        
        if (water_level > 0.02f) {
            t.water_damage += water_level;
        } else {
            t.water_damage = std::max(0.0f, t.water_damage - 0.05f);
        }
        
        float max_water_damage = 1.0f + t.canopy_density * 9.0f;
        
        if (t.water_damage > max_water_damage || water_level > 1.5f) {
            to_kill_water.push_back(uid);
            continue; 
        }
        
        Tile& soil_tile = map.get_tile_mut(t.x, t.y, t.z - 1);
        float wilting = soil_tile.wilting_point();
        if (soil_tile.state.liquid_volume > wilting) {
            float uptake = std::min(0.01f * t.canopy, soil_tile.state.liquid_volume - wilting);
            soil_tile.state.liquid_volume -= uptake;
        }
        
        float total_shade = 0.0f;
        int max_radius = 4;
        for (int dy = -max_radius; dy <= max_radius; ++dy) {
            for (int dx = -max_radius; dx <= max_radius; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = t.x + dx;
                int ny = t.y + dy;
                
                if (nx >= 0 && nx < map.get_width() && ny >= 0 && ny < map.get_height()) {
                    for (ObjectUID other_uid : obj_mgr.spatial().get_at(nx, ny, t.z)) {
                        auto it = custom_trees.find(other_uid);
                        if (it != custom_trees.end()) {
                            const TestTree& ot = it->second;
                            float dist_sq = (float)(dx*dx + dy*dy);
                            float overlap_dist = t.canopy + ot.canopy;
                            
                            if (dist_sq < overlap_dist * overlap_dist) {
                                if (ot.canopy_density > t.canopy_density || ot.canopy > t.canopy) {
                                    float dist = std::sqrt(dist_sq);
                                    float overlap_amount = overlap_dist - dist;
                                    float shade_from_tree = std::min(1.5f, overlap_amount * ot.canopy_density);
                                    total_shade += shade_from_tree;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if (total_shade > 0.0f) {
            t.canopy -= total_shade * 0.2f;
            t.canopy = std::max(0.5f, t.canopy);
        }
        
        if (total_shade > 2.5f) {
            to_kill_shade.push_back(uid);
        }
    }
    
    for (auto uid : to_kill_water) {
        obj_mgr.kill(uid, &map);
        custom_trees.erase(uid);
        global_trees_died_water++;
    }
    for (auto uid : to_kill_shade) {
        obj_mgr.kill(uid, &map);
        custom_trees.erase(uid);
        global_trees_died_shade++;
    }
}

int main() {
#ifdef _WIN32
    // Enable ANSI escape codes on Windows console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

    std::cout << "Initializing Simulation...\n";
    Game_map map(MAP_WIDTH, MAP_HEIGHT, MAP_DEPTH);

    FastNoiseLite noise;
    noise.SetSeed(SEED);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetFrequency(0.005f);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            float noise_val = noise.GetNoise((float)x, (float)y);
            int terrain_height = static_cast<int>((noise_val + 1.0f) * 0.5f * 70) + 5;
            for (int z = 0; z < MAP_DEPTH; z++) {
                if (z < terrain_height) {
                    if (z > terrain_height - 3) {
                        Tile t(MaterialType::SOIL_BASE);
                        t.state.liquid_volume = t.field_capacity() * 0.5f;
                        map.set_tile(x, y, z, t);
                    } else {
                        map.set_tile(x, y, z, Tile(MaterialType::STONE_BASE));
                    }
                } else {
                    map.set_tile(x, y, z, Tile(MaterialType::AIR));
                }
            }
        }
    }
    
    EventBus event_bus;
    event_bus.subscribe(EventType::OBJECT_KILLED, [](const EventPayload& payload) {
        custom_trees.erase(payload.source_uid);
    });

    ObjectPrototypeDB proto_db;
    proto_db.load_defaults();
    ObjectManager obj_mgr(proto_db, event_bus);
    
    MapSimulator simulator;
    simulator.initialize(map, SEED);

    // Initial populate before rain
    test_populate(map, obj_mgr, 0, simulator.get_ground_z());

    // HEAVY RAIN Phase
    int rain_years = 4;
    simulator.get_params_mut().ocean_humidity = 0.8f;      // More moisture inflow
    simulator.get_params_mut().rain_out_fraction = 0.95f;  // Almost all excess drops
    simulator.get_params_mut().convective_intensity = 0.5f; // Huge storms
    simulator.get_params_mut().convective_threshold = 0.05f; // Constant storms
    simulator.get_params_mut().evap_coeff = 0.01f; // Very low evaporation
    simulator.update_params(); // Apply to climate and groundwater!

    std::cout << "--- HEAVY RAIN PHASE (" << rain_years << " years) ---\n";
    for(int y = 0; y < rain_years; ++y) {
        simulator.reset_rainfall_tracker();
        int substeps = simulator.get_params().substeps_per_year;
        for(int s = 0; s < substeps; ++s) {
            simulator.simulate_substep(map, s, y);
        }
        
        obj_mgr.tick(&map, true);
        test_tick_trees(map, obj_mgr);
        test_populate(map, obj_mgr, y+1, simulator.get_ground_z());
        
        float last_rain_total = simulator.get_last_year_rainfall();
        float avg_rain_mm = (last_rain_total / (MAP_WIDTH * MAP_HEIGHT)) * 1000.0f;
        std::cout << "Year " << y+1 << " average rainfall: " << avg_rain_mm << " mm\n";
    }

    auto count_stats = [&]() {
        int water_tiles = 0;
        const auto& ground_z = simulator.get_ground_z();
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                int z = ground_z[y * MAP_WIDTH + x];
                if (z + 1 < MAP_DEPTH) {
                    const Tile& t = map.get_tile(x, y, z + 1);
                    if (t.material == MaterialType::AIR && t.state.liquid_volume > 0.01f) {
                        water_tiles++;
                    }
                }
            }
        }
        return water_tiles;
    };

    std::cout << "\n--- AFTER HEAVY RAIN ---\n";
    std::cout << "Total Water Tiles (Surface Ponding): " << count_stats() << "\n";
    std::cout << "Total Trees on Map: " << obj_mgr.get_all_active().size() << "\n";
    std::cout << "\nMap Sample:\n";
    print_map_slice(map, obj_mgr, VIEW_START_X, VIEW_START_Y, VIEW_WIDTH, VIEW_HEIGHT);

    // DROUGHT Phase
    int drought_years = 2; // Less than rain years
    simulator.get_params_mut().ocean_humidity = 0.01f;       // A little moisture
    simulator.get_params_mut().rain_out_fraction = 0.1f;   // Very little of evaporated water rains down
    simulator.get_params_mut().convective_intensity = 0.1f; // Weak storms
    simulator.get_params_mut().convective_threshold = 0.8f; // Rare storms
    simulator.get_params_mut().evap_coeff = 0.15f; // Milder evaporation so it doesn't instantly dry the oceans
    simulator.update_params(); // Apply!
    
    std::cout << "\n--- DROUGHT PHASE (" << drought_years << " years) ---\n";
    for(int y = 0; y < drought_years; ++y) {
        simulator.reset_rainfall_tracker();
        int substeps = simulator.get_params().substeps_per_year;
        for(int s = 0; s < substeps; ++s) {
            simulator.simulate_substep(map, s, rain_years + y);
        }
        
        obj_mgr.tick(&map, true);
        test_tick_trees(map, obj_mgr);
        test_populate(map, obj_mgr, rain_years + y + 1, simulator.get_ground_z());
        
        float last_rain_total = simulator.get_last_year_rainfall();
        float avg_rain_mm = (last_rain_total / (MAP_WIDTH * MAP_HEIGHT)) * 1000.0f;
        std::cout << "Year " << rain_years + y + 1 << " average rainfall: " << avg_rain_mm << " mm\n";
    }
    
    std::cout << "\n--- AFTER DROUGHT ---\n";
    std::cout << "Total Water Tiles (Surface Ponding): " << count_stats() << "\n";
    std::cout << "Total Trees on Map: " << obj_mgr.get_all_active().size() << "\n";
    std::cout << "Total Trees died from water: " << global_trees_died_water << "\n";
    std::cout << "\nMap Sample:\n";
    print_map_slice(map, obj_mgr, VIEW_START_X, VIEW_START_Y, VIEW_WIDTH, VIEW_HEIGHT);
    
    return 0;
}
