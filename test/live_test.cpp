#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <conio.h> // For _kbhit() on Windows
#ifdef _WIN32
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
constexpr int SPAWN_INTERVAL = 5; // Spawn trees every N years
constexpr int MAX_SPAWN_YEAR = 50; // Stop naturally spawning trees after this year

constexpr int VIEW_START_X = 35;
constexpr int VIEW_START_Y = 35;
constexpr int VIEW_WIDTH = 100;
constexpr int VIEW_HEIGHT = 40;
// -------------------------------

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
            
            out += "\033[38;5;" + std::to_string(fg) + "m" + glyph;
        }
        out += "\033[0m\n";
    }
    std::cout << out;
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
    std::cout << "Starting simulation loop. Press any key to stop.\n";
    
    while (true) {
        if (_kbhit()) {
            break;
        }
        
        auto year_start = std::chrono::steady_clock::now();
        
        // Simulate one year
        int substeps = simulator.get_params().substeps_per_year;
        for (int s = 0; s < substeps; ++s) {
            simulator.simulate_substep(map, s, year);
        }
        
        // Tick objects
        obj_mgr.tick(&map);
        
        // Spawn objects on a given interval, up to a limit
        if (year <= MAX_SPAWN_YEAR && year % SPAWN_INTERVAL == 0) {
            ObjectSpawner::populate(map, proto_db, obj_mgr, SEED + year, simulator.get_ground_z());
        }
        
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
        
        int active_trees = obj_mgr.get_all_active().size();
        
        // Clear screen
        std::cout << "\033[2J\033[1;1H";
        
        std::cout << "\033[1;36m=== Live Simulation: Year " << year << " ===\033[0m\n";
        std::cout << "Total Water Tiles: " << water_tiles << "\n";
        std::cout << "Avg Soil Moisture: " << (soil_tiles > 0 ? total_moisture / soil_tiles : 0.0f) << "\n";
        std::cout << "Total Trees (Objects): " << active_trees << "\n";
        std::cout << "Atmospheric Vapor: " << simulator.total_atmospheric_vapor() << "\n";
        std::cout << "\n\033[1;33mMap Slice (" << VIEW_WIDTH << "x" << VIEW_HEIGHT << " at " << VIEW_START_X << "," << VIEW_START_Y << "):\033[0m\n";
        
        print_map_slice(map, obj_mgr, VIEW_START_X, VIEW_START_Y, VIEW_WIDTH, VIEW_HEIGHT);
        
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
