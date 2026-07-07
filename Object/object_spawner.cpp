#include <cmath>
#include "object_spawner.hpp"
#include "../game_map.hpp"
#include <cstdlib>
#include <cstdio>

namespace ObjectSpawner {

// Simple seeded random [0, 1)
static float rand_float(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (state & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

void populate(Game_map& map, ObjectPrototypeDB& proto_db,
              ObjectManager& obj_mgr, int seed,
              const std::vector<int>& ground_z_in) {
    int width  = map.get_width();
    int height = map.get_height();
    int depth  = map.get_depth();

    // Build ground heightmap if not provided
    std::vector<int> ground_z;
    if (ground_z_in.empty()) {
        ground_z.resize(width * height, 0);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int gz = depth - 1;
                while (gz > 0 && map.get_tile(x, y, gz).material == MaterialType::AIR
                       && map.get_tile(x, y, gz).state.liquid_volume <= 0.0f) {
                    gz--;
                }
                // gz is now the topmost non-air tile (solid ground)
                ground_z[y * width + x] = gz;
            }
        }
    } else {
        ground_z = ground_z_in;
    }

    int total_spawned = 0;

    // Iterate over every prototype in the DB
    for (uint16_t proto_id = 1; proto_id < (uint16_t)proto_db.size(); ++proto_id) {
        const auto& proto = proto_db.get(proto_id);
        const auto& req = proto.spawn_req;

        // Skip prototypes with zero probability (items, corpses, etc.)
        if (req.probability <= 0.0f) continue;

        // Per-prototype seed so each type gets reproducible placement
        uint32_t rng_state = (uint32_t)seed ^ ((uint32_t)proto_id * 2654435761u);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int gz = ground_z[y * width + x];
                int spawn_z = gz + 1;  // Object sits on top of ground

                // --- Check SpawnRequirements ---

                // Elevation
                if (gz < req.min_z || gz > req.max_z) continue;

                // Must have room above ground
                if (spawn_z >= depth) continue;

                float current_prob = req.probability;

                // Ground material
                if (req.requires_ground) {
                    const Tile& ground = map.get_tile(x, y, gz);
                    if (ground.material != req.ground_material) continue;

                    // Soil moisture
                    float moisture = ground.state.liquid_volume;
                    if (moisture < req.min_soil_moisture) continue;
                    if (moisture > req.max_soil_moisture) continue;
                    
                    if (proto.behavior == ObjectBehavior::VEGETATION) {
                        // Normalize moisture to [0, 1] within their allowed range
                        float range = req.max_soil_moisture - req.min_soil_moisture;
                        float normalized = (range > 0.001f) ? (moisture - req.min_soil_moisture) / range : 0.0f;
                        
                        // Scale probability: baseline at min moisture, up to 4x higher near ideal moisture (30-50% normalized)
                        float ideal_factor = 1.0f - std::fabs(normalized - 0.4f) * 2.0f;
                        ideal_factor = std::max(0.0f, ideal_factor);
                        current_prob = req.probability * (1.0f + 3.0f * ideal_factor);
                    }

                    // Temperature
                    float temp = ground.state.temperature;
                    if (temp < req.min_temp || temp > req.max_temp) continue;
                }

                // The spawn tile itself must be air (not already occupied by solid)
                const Tile& trunk_tile = map.get_tile(x, y, spawn_z);
                if (trunk_tile.material != MaterialType::AIR) continue;

                // Don't spawn trees inside deep water
                if (proto.behavior == ObjectBehavior::VEGETATION) {
                    if (trunk_tile.state.liquid_volume > 0.05f) continue;
                }

                // Don't stack objects -- skip if something is already here
                if (obj_mgr.spatial().has_any(x, y, spawn_z)) continue;

                // Probability roll
                if (rand_float(rng_state) > current_prob) continue;

                // All checks passed -- spawn the object
                obj_mgr.spawn(proto_id, x, y, spawn_z);
                total_spawned++;
            }
        }
    }

    std::printf("[ObjectSpawner] Spawned %d objects.\n", total_spawned);
}

}  // namespace ObjectSpawner
