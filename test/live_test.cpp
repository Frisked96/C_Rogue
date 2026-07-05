#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <conio.h> // For _kbhit() on Windows
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
#include "../Object/object_prototype_db.hpp"
#include "../Object/object_manager.hpp"
#include "../Object/event_bus.hpp"
#include "../Object/object_spawner.hpp"

// --- Configuration Constants ---
constexpr int MAP_WIDTH = 200;
constexpr int MAP_HEIGHT = 100;
constexpr int MAP_DEPTH = 150;

constexpr int SEED = 12345;
constexpr int SLEEP_MS = 50;
constexpr int SPAWN_INTERVAL = 10; // Spawn trees every N years
constexpr int MAX_SPAWN_YEAR = 50; // Stop naturally spawning trees after this year

constexpr int VIEW_START_X = 35;
constexpr int VIEW_START_Y = 35;
constexpr int VIEW_WIDTH = 100;
constexpr int VIEW_HEIGHT = 40;
// -------------------------------

// --- Custom Test Tree Logic ---
struct TestTree {
    int x, y, z;
    int age = 0;
    float canopy = 1.0f;
    float canopy_density = 0.1f;
    int max_age = 0;
};

std::unordered_map<ObjectUID, TestTree> custom_trees;

// Utility function to print a small top-down slice of the map
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
                                if (custom_trees[uid].age < 15 || custom_trees[uid].canopy <= 2.0f) {
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
                out += "\033[38;5;" + std::to_string(fg) + ";" + std::to_string(bg) + "m" + glyph;
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
            
            // Height constraints: only spawn on elevations between 15 and 60
            if (gz < 15 || gz > 60) continue;
            
            int spawn_z = gz + 1;
            if (spawn_z >= MAP_DEPTH) continue;
            const Tile& trunk_tile = map.get_tile(x, y, spawn_z);
            if (trunk_tile.material != MaterialType::AIR) continue;
            if (trunk_tile.state.liquid_volume > 0.05f) continue; // Don't spawn in lakes or flowing rivers
            
            const Tile& ground = map.get_tile(x, y, gz);
            if (ground.material != MaterialType::SOIL_BASE) continue;
            
            float moisture = ground.state.liquid_volume;
            if (moisture < 0.08f || moisture > 0.6f) continue; // No dust, no mud
            
            float prob = 0.02f;
            if (moisture > 0.25f && moisture < 0.45f) prob = 0.08f; // Ideal zone
            
            if ((float)rand() / RAND_MAX > prob) continue;
            if (obj_mgr.spatial().has_any(x, y, spawn_z)) continue;
            
            ObjectUID uid = obj_mgr.spawn(proto_id, x, y, spawn_z);
            TestTree t;
            t.x = x; t.y = y; t.z = spawn_z;
            t.max_age = 40 + (rand() % 40); // Die after 40-80 years (disabled per request)
            custom_trees[uid] = t;
        }
    }
}

void test_tick_trees(Game_map& map, ObjectManager& obj_mgr, int& total_trees_died) {
    std::vector<ObjectUID> to_kill;
    
    // Reset all flow blockage (since trees can die, we recalculate it)
    for (int y = 0; y < map.get_height(); ++y) {
        for (int x = 0; x < map.get_width(); ++x) {
            map.get_surface(x, y).flow_blockage = 0.0f;
        }
    }
    
    for (auto& pair : custom_trees) {
        ObjectUID uid = pair.first;
        TestTree& t = pair.second;
        
        t.age++;
        // Canopy grows slower, max 2.0 tiles radius
        t.canopy = std::min(2.0f, 1.0f + t.age * 0.02f);
        // Density increases with age, max 1.0
        t.canopy_density = std::min(1.0f, t.age * 0.01f);
        
        // 1. Drown Check (if tree trunk is in > 0.15m of water)
        const Tile& trunk_tile = map.get_tile(t.x, t.y, t.z);
        if (trunk_tile.state.liquid_volume > 0.15f) {
            to_kill.push_back(uid);
            continue; // Uprooted/drowned by river!
        }
        
        // 2. Drink Water
        Tile& soil_tile = const_cast<Tile&>(map.get_tile(t.x, t.y, t.z - 1));
        float wilting = soil_tile.wilting_point();
        if (soil_tile.state.liquid_volume > wilting) {
            // Sucks up to 0.01m per year based on canopy size
            float uptake = std::min(0.01f * t.canopy, soil_tile.state.liquid_volume - wilting);
            soil_tile.state.liquid_volume -= uptake;
        }
        
        // 3. Affect Flow
        // Tree trunk & roots block surface water. Larger density = more blockage (up to 0.8)
        map.get_surface(t.x, t.y).flow_blockage = std::min(0.8f, t.canopy_density * 0.8f);
        
        // Immortal trees: No max_age death logic!
        
        float total_shade = 0.0f;
        for (const auto& other_pair : custom_trees) {
            if (other_pair.first == uid) continue;
            const TestTree& ot = other_pair.second;
            float dx = t.x - ot.x;
            float dy = t.y - ot.y;
            float dist_sq = dx*dx + dy*dy;
            float overlap_dist = t.canopy + ot.canopy;
            
            if (dist_sq < overlap_dist * overlap_dist) {
                // Shade is cast by taller/denser trees
                if (ot.canopy_density > t.canopy_density || ot.canopy > t.canopy) {
                    float dist = std::sqrt(dist_sq);
                    float overlap_amount = overlap_dist - dist;
                    // Max shade from one tree is capped so one tree cannot kill another alone
                    float shade_from_tree = std::min(1.5f, overlap_amount * ot.canopy_density);
                    total_shade += shade_from_tree;
                }
            }
        }
        
        // Shade hinders growth (reduces canopy size temporarily)
        if (total_shade > 0.0f) {
            t.canopy -= total_shade * 0.2f;
            t.canopy = std::max(0.5f, t.canopy); // never shrink below 0.5
        }
        
        // Requires significant shade from MULTIPLE trees to die (e.g. > 2.5 shade total)
        if (total_shade > 2.5f) {
            to_kill.push_back(uid);
        }
    }
    
    for (auto uid : to_kill) {
        obj_mgr.kill(uid);
        custom_trees.erase(uid);
        total_trees_died++;
    }
}
// ------------------------------

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

    std::cout << "Initializing Live Test Simulation...\n";
    
    Game_map map(MAP_WIDTH, MAP_HEIGHT, MAP_DEPTH);
    
    // Generate base terrain
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
    ObjectPrototypeDB proto_db;
    proto_db.load_defaults();
    ObjectManager obj_mgr(proto_db, event_bus);
    
    MapSimulator simulator;
    simulator.initialize(map, SEED);
    
    int year = 0;
    int total_trees_died = 0;
    int years_rained = 0;
    float prev_year_rain = 0.0f;
    std::cout << "Starting simulation loop. Press any key to stop.\n";
    
    while (true) {
        if (_kbhit()) {
            break;
        }
        
        auto year_start = std::chrono::steady_clock::now();
        
        simulator.reset_rainfall_tracker();
        
        float vapor_before = simulator.total_atmospheric_vapor();
        
        // Simulate one year
        int substeps = simulator.get_params().substeps_per_year;
        for (int s = 0; s < substeps; ++s) {
            simulator.simulate_substep(map, s, year);
        }
        
        float vapor_after = simulator.total_atmospheric_vapor();
        float this_year_rain = simulator.get_last_year_rainfall();
        
        // Tick objects
        obj_mgr.tick(&map);
        
        // Custom test aging & canopy logic
        test_tick_trees(map, obj_mgr, total_trees_died);
        
        // Custom test spawning logic
        test_populate(map, obj_mgr, year, simulator.get_ground_z());
        
        // Compute stats
        int water_tiles = 0;
        float total_moisture = 0;
        int soil_tiles = 0;
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                int z = simulator.get_ground_z()[y * MAP_WIDTH + x];
                if (z + 1 < MAP_DEPTH) {
                    const Tile& t = map.get_tile(x, y, z + 1);
                    if (t.material == MaterialType::AIR && t.state.liquid_volume > 0.0f) {
                        water_tiles++;
                    }
                }
                const Tile& ground = map.get_tile(x, y, z);
                if (ground.material == MaterialType::SOIL_BASE) {
                    soil_tiles++;
                    total_moisture += ground.state.liquid_volume;
                }
            }
        }
        
        if (this_year_rain > 0.0f) {
            years_rained++;
        }
        
        int active_trees = obj_mgr.get_all_active().size();
        
        // Clear screen
        std::cout << "\033[2J\033[1;1H";
        
        std::cout << "\033[1;36m=== Live Simulation: Year " << year << " ===\033[0m\n";
        std::cout << "Total Water Tiles: " << water_tiles << "\n";
        std::cout << "Avg Soil Moisture: " << (soil_tiles > 0 ? total_moisture / soil_tiles : 0.0f) << "\n";
        std::cout << "Total Trees (Living): " << active_trees << " | Total Trees Died: " << total_trees_died << "\n";
        std::cout << "Rain This Year: " << this_year_rain << " | Rain Last Year: " << prev_year_rain << " | Total Years Rained: " << years_rained << "\n";
        std::cout << "\n\033[1;33mMap Slice (" << VIEW_WIDTH << "x" << VIEW_HEIGHT << " at " << VIEW_START_X << "," << VIEW_START_Y << "):\033[0m\n";
        
        print_map_slice(map, obj_mgr, VIEW_START_X, VIEW_START_Y, VIEW_WIDTH, VIEW_HEIGHT);
        
        prev_year_rain = this_year_rain;
        year++;
        
        // Sleep to cap CPU
        auto year_end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(year_end - year_start).count();
        if (duration < SLEEP_MS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS - duration));
        }
    }
    
    std::cout << "Simulation stopped.\n";
    return 0;
}
