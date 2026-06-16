#include "game_map.hpp"
#include "map_gen/map_generator.hpp"
#include "map_gen/simulator.hpp"
#include <algorithm>

Game_map::Game_map(int w, int h, int d)
    : width(w), height(h), depth(d),
      map(w * h * d, Tiles::Wall),
      surface(w * h)  // default‑initialised to zero
{}

const Tile& Game_map::get_tile(int x, int y, int z) const {
    if (is_in_bounds(x, y, z))
        return map[get_index(x, y, z)];
    return Tiles::Wall;
}

void Game_map::set_tile(int x, int y, int z, const Tile& tile) {
    if (is_in_bounds(x, y, z))
        map[get_index(x, y, z)] = tile;
}

bool Game_map::can_walk(int x, int y, int z) const {
    if (!is_in_bounds(x, y, z)) return false;
    return get_tile(x, y, z).get_is_walkable();
}

bool Game_map::is_in_bounds(int x, int y, int z) const {
    return x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth;
}

// --- Surface cover ---
SurfaceCover& Game_map::get_surface(int x, int y) {
    // No z – surface layer is always 2D
    return surface[y * width + x];
}
const SurfaceCover& Game_map::get_surface(int x, int y) const {
    return surface[y * width + x];
}

// --- Sparse soil chemistry ---
SoilChemistry& Game_map::get_soil_chemistry(int x, int y, int z) {
    int idx = get_index(x, y, z);
    return soil_chem[idx];   // default‑constructed if missing
}
const SoilChemistry& Game_map::get_soil_chemistry(int x, int y, int z) const {
    static SoilChemistry default_chem;
    auto it = soil_chem.find(get_index(x, y, z));
    return (it != soil_chem.end()) ? it->second : default_chem;
}
bool Game_map::has_soil_chemistry(int x, int y, int z) const {
    return soil_chem.find(get_index(x, y, z)) != soil_chem.end();
}
void Game_map::erase_soil_chemistry(int x, int y, int z) {
    soil_chem.erase(get_index(x, y, z));
}

// --- Sparse structural integrity ---
float Game_map::get_structural_integrity(int x, int y, int z) const {
    auto it = structural_int.find(get_index(x, y, z));
    return (it != structural_int.end()) ? it->second : 1.0f;
}
void Game_map::set_structural_integrity(int x, int y, int z, float value) {
    structural_int[get_index(x, y, z)] = std::clamp(value, 0.0f, 1.0f);
}
void Game_map::erase_structural_integrity(int x, int y, int z) {
    structural_int.erase(get_index(x, y, z));
}

// --- High‑level query using sparse soil data ---
bool Game_map::can_plant(int x, int y, int z) const {
    if (!is_in_bounds(x, y, z)) return false;
    const Tile& tile = get_tile(x, y, z);

    // Physical requirements (temperature & moisture)
    if (tile.state.temperature <= 278.0f) return false;
    if (tile.state.liquid_volume <= tile.wilting_point()) return false;

    // Soil fertility requirement – only meaningful for soil‑like materials
    const SoilChemistry& chem = get_soil_chemistry(x, y, z);
    // Basic check: average nutrient level and pH range
    float avg_nutrient = (chem.nutrients_N + chem.nutrients_P + chem.nutrients_K) / 3.0f;
    if (avg_nutrient < 0.05f || chem.pH < 4.0f || chem.pH > 9.0f) return false;
    if (chem.contamination > 0.8f) return false;

    // Organic matter not strictly required, but very low may hinder growth
    // (can be tuned)
    return true;
}

// --- Visibility (unchanged) ---
void Game_map::clear_visibility() {
    for (auto& tile : map) tile.is_visible = false;
}
bool Game_map::is_opaque(int x, int y, int z) const {
    if (!is_in_bounds(x, y, z)) return true;
    return get_tile(x, y, z).mat().is_opaque;
}
void Game_map::set_visible(int x, int y, int z, bool visible) {
    if (is_in_bounds(x, y, z)) {
        int idx = get_index(x, y, z);
        map[idx].is_visible = visible;
        if (visible) map[idx].is_explored = true;
    }
}
bool Game_map::is_visible(int x, int y, int z) const {
    if (!is_in_bounds(x, y, z)) return false;
    return map[get_index(x, y, z)].is_visible;
}
bool Game_map::is_explored(int x, int y, int z) const {
    if (!is_in_bounds(x, y, z)) return false;
    return map[get_index(x, y, z)].is_explored;
}

void Game_map::generate(int seed) {
    MapGenerator::generate(*this, seed);
}