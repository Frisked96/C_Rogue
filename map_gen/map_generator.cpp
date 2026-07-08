#include "map_generator.hpp"
#include "../material.hpp"
#include "simulator.hpp"
#include <cmath>

void MapGenerator::generate(Game_map &game_map, int seed,
                            ObjectPrototypeDB *proto_db,
                            ObjectManager *obj_mgr) {
  generate_terrain(game_map, seed);
  simulate_hydrology(game_map, seed, proto_db, obj_mgr);
}

void MapGenerator::generate_terrain(Game_map &game_map, int seed) {
  FastNoiseLite noise;
  noise.SetSeed(seed);
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(3);
  noise.SetFrequency(0.005f); // Broad landforms

  int width = game_map.get_width();
  int height = game_map.get_height();
  int depth = game_map.get_depth();

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float noise_val = noise.GetNoise((float)x, (float)y);

      // Map noise to terrain height
      int terrain_height =
          static_cast<int>((noise_val + 1.0f) * 0.5f * (depth * 0.7f)) + 5;

      for (int z = 0; z < depth; z++) {
        Tile t;
        if (z < terrain_height) {
          if (z > terrain_height - 3) {
            t = Tile(MaterialType::SOIL_BASE);

            // Give the topsoil some initial moisture so it isn't instantly
            // dust before the first rain falls. Field capacity is a safe start.
            t.state.liquid_volume = t.field_capacity() * 0.5f;
          } else {
            t = Tile(MaterialType::STONE_BASE);
            // Bedrock starts completely dry. The simulator's abstract
            // GroundwaterGrid will handle filling it conceptually over time.
          }
        } else {
          t = Tile(MaterialType::AIR);
          // NO PRE-FILLED OCEANS OR LAKES. The simulator will fill these
          // basins naturally via precipitation and overland flow.
        }
        game_map.set_tile(x, y, z, t);
      }
    }
  }
}

void MapGenerator::simulate_hydrology(Game_map &game_map, int seed,
                                      ObjectPrototypeDB *proto_db,
                                      ObjectManager *obj_mgr) {
  // Run the hydrology simulation to generate rivers, lakes, and groundwater
  MapSimulator simulator;
  simulator.run(game_map, seed, 65, proto_db, obj_mgr);
}
