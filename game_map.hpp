#pragma once

#include "tile.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

// Surface cover stored only on the topmost (ground) layer (2D)
struct SurfaceCover {
    float snow_depth  = 0.0f;   // metres
    float litter_mass = 0.0f;   // kg
};

// Soil chemistry & contamination – stored sparsely, only for tiles where it matters
struct SoilChemistry {
    float nutrients_N    = 0.5f;
    float nutrients_P    = 0.5f;
    float nutrients_K    = 0.5f;
    float organic_matter = 0.1f;
    float pH             = 7.0f;
    float contamination  = 0.0f;
    float sediment       = 0.0f;  // eroded material currently suspended in tile’s water
};

class Game_map {
public:
    Game_map(int w, int h, int d);

    // --- 3D tile access ---
    const Tile& get_tile(int x, int y, int z) const;
    void        set_tile(int x, int y, int z, const Tile& tile);
    bool        can_walk(int x, int y, int z) const;
    bool        is_in_bounds(int x, int y, int z) const;

    // --- Surface cover (2D) ---
    SurfaceCover&       get_surface(int x, int y);
    const SurfaceCover& get_surface(int x, int y) const;

    // --- Sparse soil chemistry ---
    SoilChemistry&       get_soil_chemistry(int x, int y, int z);
    const SoilChemistry& get_soil_chemistry(int x, int y, int z) const;
    bool                 has_soil_chemistry(int x, int y, int z) const;
    void                 erase_soil_chemistry(int x, int y, int z);

    // --- Sparse structural integrity ---
    float  get_structural_integrity(int x, int y, int z) const;
    void   set_structural_integrity(int x, int y, int z, float value);
    void   erase_structural_integrity(int x, int y, int z);

    // --- High‑level queries (using sparse data) ---
    bool can_plant(int x, int y, int z) const;

    // --- Dimensions ---
    int get_height() const { return height; }
    int get_width()  const { return width; }
    int get_depth()  const { return depth; }

    // --- Map generation ---
    void generate(int seed = 1337);

    // --- Visibility ---
    void clear_visibility();
    bool is_opaque(int x, int y, int z) const;
    void set_visible(int x, int y, int z, bool visible);
    bool is_visible(int x, int y, int z) const;
    bool is_explored(int x, int y, int z) const;

private:
    int width, height, depth;
    std::vector<Tile> map;                      // 3D voxel grid
    std::vector<SurfaceCover> surface;          // 2D top layer (width*height)
    std::unordered_map<int, SoilChemistry> soil_chem;   // key = linearised index
    std::unordered_map<int, float> structural_int;     // key = linearised index

    int get_index(int x, int y, int z) const {
        return (z * width * height) + (y * width) + x;
    }
};