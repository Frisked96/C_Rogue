#include "map_generator.hpp"
#include "simulator.hpp"
#include "../material.hpp"

void MapGenerator::generate(Game_map& game_map, int seed) {
    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetFrequency(0.01f); // Lower frequency for smoother, larger hills

    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float noise_val = noise.GetNoise((float)x, (float)y);
            
            // Map noise (-1 to 1) to height
            int terrain_height = static_cast<int>((noise_val + 1.0f) * 0.5f * (depth * 0.6f)) + 5;
            
            for (int z = 0; z < depth; z++) {
                if (z < terrain_height) {
                    // Place base materials
                    if (z > terrain_height - 3) {
                        game_map.set_tile(x, y, z, Tile(MaterialType::SOIL_BASE));
                    } else {
                        game_map.set_tile(x, y, z, Tile(MaterialType::STONE_BASE));
                    }
                } else {
                    game_map.set_tile(x, y, z, Tile(MaterialType::AIR));
                }
            }
        }
    }

    // Run the hydrology simulation
    MapSimulator::run(game_map, seed);
}
