#include "map_generator.hpp"
#include "../material.hpp"

void MapGenerator::generate(Game_map& game_map, int seed) {
    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.02f); // Low frequency for flat-ish terrain

    int width = game_map.get_width();
    int height = game_map.get_height();
    int depth = game_map.get_depth();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float noise_val = noise.GetNoise((float)x, (float)y);
            
            // Map noise (-1 to 1) to height (e.g., 2 to 5 for a 10-deep map)
            int terrain_height = static_cast<int>((noise_val + 1.0f) * 0.5f * (depth / 2.0f)) + 1;
            
            for (int z = 0; z < depth; z++) {
                if (z < terrain_height) {
                    MaterialType mat = get_material_for_height(z, terrain_height, noise_val);
                    game_map.set_tile(x, y, z, Tile(mat));
                } else {
                    game_map.set_tile(x, y, z, Tile(MaterialType::AIR));
                }
            }
        }
    }
}

MaterialType MapGenerator::get_material_for_height(int z, int terrain_height, float noise_val) {
    if (z == terrain_height - 1) {
        // Surface layer
        if (noise_val > 0.5f) return MaterialType::STONE_GRANITE; // Stony peaks
        if (noise_val < -0.2f) return MaterialType::SOIL_SAND;   // Sandy lowlands
        return MaterialType::SOIL_LOAM;                          // Standard soil
    }
    
    // Deep layers
    if (z < terrain_height - 2) {
        return MaterialType::STONE_GRANITE;
    }
    
    return MaterialType::SOIL_CLAY;
}
