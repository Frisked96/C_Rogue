#pragma once

// =============================================================================
// climate_hydrology.hpp
//
// A layered climate / hydrology system that replaces the old monolithic
// "global humidity + uniform raindrops + *100 pressure flow" simulation.
//
// Three linked layers, as described in the design notes:
//
//   1. Atmosphere (ClimateSystem)  - 2D fields of wind, temperature and
//      column water-vapour ("vapor"). Vapor is advected by wind, lost to
//      orographic + convective precipitation, and gained back via
//      evapotranspiration.
//
//   2. Land surface & soil column (free functions operating on Game_map)
//      - a "bucket model": infiltration, field-capacity/wilting-point aware
//      percolation (K(theta) ~ theta^3), capillary rise, and surface ponding.
//
//   3. Groundwater & surface water (GroundwaterGrid + overland_flow_step)
//      - a slowly diffusing 2D water-table plus D8 steepest-descent overland
//      flow for ponded surface water, so rivers/lakes form from topography
//      instead of spreading uniformly underground.
// -----------------------------------------------------------------------
// UNITS / CONVENTIONS
//
//  - Every tile is 1x1x1 m, so a "moisture" value can be read interchangeably
//    as a volume fraction (0..capacity) OR as a depth in metres - this is the
//    same convention the original code already used for ponds.
//  - `permeability` in MaterialProperties is treated as a dimensionless
//    "game balance" coefficient in roughly [0,1] (NOT true SI m/s). All of
//    the *_scale / *_rate constants in Params below convert it into a
//    fraction-of-moisture-moved-per-substep. If your numbers feel too
//    fast/slow, tune Params - the formulas themselves are unit-agnostic.
//  - "vapor" (Q) is an abstract per-column water-equivalent depth (metres),
//    directly comparable to precipitation depth and to tile moisture.
// =============================================================================

#include "../FastNoiseLite.h"
#include "../tile.hpp"
#include <cstdint>
#include <vector>

// -----------------------------------------------------------------------
// NoiseGen: thin wrapper around FastNoiseLite providing the same interface
// previously offered by the standalone noise.hpp. FastNoiseLite.h must be
// on the include path (it is already a project dependency).
//
// Two internal FastNoiseLite instances are kept so that different octave
// counts in successive fbm2D() calls don't require a full re-init each
// time: changes are only applied when the parameters actually differ from
// the last call (single-threaded world-gen, so no locking needed).
//
// All instances use frequency = 1.0f because callers pre-scale their
// coordinates (e.g. x * 0.015f), consistent with the convention used
// throughout climate_hydrology.cpp.
// -----------------------------------------------------------------------
class NoiseGen {
public:
  NoiseGen() { init(0); }
  explicit NoiseGen(uint32_t seed) { init(seed); }

  void set_seed(uint32_t s) {
    fnl_single_.SetSeed((int)s);
    fnl_fractal_.SetSeed((int)s);
  }

  // Single-octave noise, range ~[-1, 1].
  float noise2D(float x, float y) { return fnl_single_.GetNoise(x, y); }

  // Fractal Brownian motion, range ~[-1, 1].
  float fbm2D(float x, float y, int octaves = 4, float lacunarity = 2.0f,
              float gain = 0.5f) {
    if (octaves != last_octaves_ || lacunarity != last_lacunarity_ ||
        gain != last_gain_) {
      fnl_fractal_.SetFractalOctaves(octaves);
      fnl_fractal_.SetFractalLacunarity(lacunarity);
      fnl_fractal_.SetFractalGain(gain);
      last_octaves_ = octaves;
      last_lacunarity_ = lacunarity;
      last_gain_ = gain;
    }
    return fnl_fractal_.GetNoise(x, y);
  }

  // Convenience variants mapped to [0, 1].
  float noise01(float x, float y) { return noise2D(x, y) * 0.5f + 0.5f; }
  float fbm01(float x, float y, int octaves = 4, float lacunarity = 2.0f,
              float gain = 0.5f) {
    return fbm2D(x, y, octaves, lacunarity, gain) * 0.5f + 0.5f;
  }

private:
  void init(uint32_t seed) {
    fnl_single_.SetSeed((int)seed);
    fnl_single_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl_single_.SetFrequency(1.0f);
    fnl_single_.SetFractalType(FastNoiseLite::FractalType_None);

    fnl_fractal_.SetSeed((int)seed);
    fnl_fractal_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fnl_fractal_.SetFrequency(1.0f);
    fnl_fractal_.SetFractalType(FastNoiseLite::FractalType_FBm);
    fnl_fractal_.SetFractalOctaves(4);
    fnl_fractal_.SetFractalLacunarity(2.0f);
    fnl_fractal_.SetFractalGain(0.5f);
  }

  FastNoiseLite fnl_single_;
  FastNoiseLite fnl_fractal_;
  int last_octaves_ = 4;
  float last_lacunarity_ = 2.0f;
  float last_gain_ = 0.5f;
};

class Game_map; // forward declaration only - full definition needed in the .cpp

namespace hydro {

// -----------------------------------------------------------------------
// Tunable parameters. Defaults are chosen to be Courant-safe (no single
// substep can move more than a tile's own capacity of water), per the
// "respect the Courant condition" guidance in the design notes.
// -----------------------------------------------------------------------
struct Params {
  // --- Time stepping ---
  int substeps_per_year = 120; // ~3/day; design notes suggest >=120

  // --- Atmosphere / climate ---
  float sea_level_temp_K = 288.15f;   // 15 C baseline air temperature
  float seasonal_amplitude_K = 12.0f; // +/- swing over the year
  float lapse_rate_K_per_tile =
      0.45f; // "gamified" lapse rate (K lost per metre of elevation)
  float orographic_gain =
      4.0f;               // extra cooling per unit of upslope wind component
  float qsat_ref = 0.05f; // saturation vapor "capacity" at sea_level_temp_K
  float qsat_k =
      0.07f; // exponential rate, per Kelvin (Clausius-Clapeyron-like)
  float rain_out_fraction =
      0.35f; // fraction of supersaturation that rains out per substep
  float ocean_humidity = 0.05f; // boundary inflow humidity (windward edges)
  float boundary_relax =
      0.25f; // how fast windward edge cells relax toward ocean_humidity
  float convective_threshold = 0.6f; // noise threshold for spontaneous storms
  float convective_intensity =
      0.04f; // extra rain depth at full storm intensity
  float wind_advect_scale =
      0.4f; // converts wind "speed" units into fraction-of-vapor-moved
  int wind_update_interval = 10; // substeps between wind-field refreshes

  // --- Soil hydrology (bucket model) ---
  float percolation_rate_scale =
      1.2f; // multiplies permeability-derived K(theta)
  float capillary_rate =
      0.05f; // fraction of vertical moisture gradient exchanged/substep
  float soil_variation_amplitude =
      0.2f; // +/- fractional variation in porosity/capacity per column

  // --- Surface water (D8 overland flow) ---
  float overland_flow_fraction =
      0.35f; // fraction of head-difference moved per substep

  // --- Groundwater ---
  float groundwater_porosity =
      0.2f; // converts recharge volume <-> water-table height
  float groundwater_diffusion_rate =
      0.08f; // fraction of neighbour-difference smoothed per update
  int groundwater_update_interval = 10; // substeps between groundwater updates

  // --- Evaporation / evapotranspiration ---
  float evap_coeff =
      0.08f; // overall rate multiplier for VPD-driven evaporation
};

// -----------------------------------------------------------------------
// Shared helper math
// -----------------------------------------------------------------------

// Saturation "vapor capacity" at a given air temperature (Kelvin), using a
// simple exponential (Clausius-Clapeyron-like) curve. Same shape is reused
// for surface evaporation (Tile temperature) and atmospheric saturation
// (ClimateCell temperature).
float qsat(float temperature_K, const Params &p);

// -----------------------------------------------------------------------
// Heightmap: topmost "ground" tile per column (excludes AIR and
// WATER_FRESH, i.e. the solid terrain surface a river could sit on top of).
// Computed once per run() since terrain does not change during simulation.
// Returns a width*height vector indexed as [y*width + x].
// -----------------------------------------------------------------------
std::vector<int> compute_ground_heightmap(const Game_map &map);

// Per-column multiplicative variation (roughly 1 +/- soil_variation_amplitude)
// applied to porosity/field-capacity/wilting-point during percolation, so
// "even within the same material type ... underground flow [is] more
// interesting" without touching Tile::effective_porosity() globally.
std::vector<float> compute_soil_variation(int width, int height,
                                          NoiseGen &noise, const Params &p);

// -----------------------------------------------------------------------
// Atmosphere layer
// -----------------------------------------------------------------------
struct ClimateCell {
  float wind_u = 0.0f; // "tiles moved per substep" in x, signed
  float wind_v = 0.0f; // "tiles moved per substep" in y, signed
  float vapor = 0.0f;  // column water vapour + cloud water (metres-equivalent)
  float temperature = 288.15f; // near-surface air temperature (K)
  float upslope =
      0.0f; // cached dot(wind, elevation gradient); >0 = ascending/windward
};

class ClimateSystem {
public:
  ClimateSystem() = default;

  void init(int width, int height, const Params &params);

  // Seed wind, temperature and initial (50% RH) vapor fields.
  void initialize(const std::vector<int> &ground_z, NoiseGen &noise);

  // Recompute the wind field (prevailing seasonal direction + noise,
  // deflected toward valleys/contours over steep terrain). Cheap-ish;
  // called every `wind_update_interval` substeps.
  void update_wind(const std::vector<int> &ground_z, NoiseGen &noise,
                   float season_phase);

  // Recompute near-surface air temperature from elevation + season.
  // Cheap; called every substep.
  void update_temperature(const std::vector<int> &ground_z, float season_phase);

  // Donor-cell upwind advection of `vapor` by the wind field, plus
  // windward-boundary inflow relaxation toward ocean_humidity.
  void advect();

  // Orographic + convective precipitation. Consumes supersaturated vapor
  // and returns precipitation depth (metres) per column for this substep.
  std::vector<float> step_precipitation(const std::vector<int> &ground_z,
                                        NoiseGen &noise, float day_index);

  // Evapotranspiration step adds water back into the local vapor field.
  void add_vapor(int x, int y, float amount);

  int width() const { return w_; }
  int height() const { return h_; }
  const ClimateCell &at(int x, int y) const { return cells_[idx(x, y)]; }

  // Diagnostic: sum of vapor across all columns.
  float total_vapor() const {
    float s = 0.0f;
    for (const auto &c : cells_)
      s += c.vapor;
    return s;
  }

private:
  int idx(int x, int y) const { return y * w_ + x; }

  int w_ = 0, h_ = 0;
  Params params_;
  std::vector<ClimateCell> cells_;
};

// -----------------------------------------------------------------------
// Groundwater layer
// -----------------------------------------------------------------------
class GroundwaterGrid {
public:
  GroundwaterGrid() = default;

  void init(int width, int height, const Params &params);

  // Initial water-table height (z-coordinate) per column, varied with
  // noise (some areas start wetter than others).
  void initialize(const std::vector<int> &ground_z, NoiseGen &noise);

  // Add recharge from deep percolation (volume in metres-equivalent).
  void recharge(int x, int y, float volume);

  // Smooths the water table toward neighbours (rate scaled by local
  // permeability sampled from `map`), and returns a per-column discharge
  // volume wherever the table has risen above ground level (springs /
  // baseflow). Caller is expected to add the returned discharge to the
  // surface pond at (x, ground_z[x,y]+1).
  std::vector<float> update(const std::vector<int> &ground_z,
                            const Game_map &map);

  float water_table(int x, int y) const { return table_[idx(x, y)]; }

  // Diagnostic: sum of water-table height across all columns.
  float total_table() const {
    float s = 0.0f;
    for (float v : table_)
      s += v;
    return s;
  }

private:
  int idx(int x, int y) const { return y * w_ + x; }

  int w_ = 0, h_ = 0;
  Params params_;
  std::vector<float> table_;
};

// -----------------------------------------------------------------------
// Surface & soil routines - operate directly on Game_map
// -----------------------------------------------------------------------

// Adds `depth` metres of water to the surface pond at column (x,y)
// (i.e. the tile at ground_z+1), filling AIR voids with liquid as needed and
// cascading further upward if that tile itself overflows. No-op if depth<=0
// or if ground_z+1 is out of bounds (solid terrain reaches the map ceiling).
void add_surface_water(Game_map &map, int x, int y, int ground_z, float depth,
                       const Params &p);

// Step 1: infiltration. Adds the precipitation field to each column's
// topmost ground tile, handling saturation overflow into a surface pond.
void apply_precipitation(Game_map &map, const std::vector<int> &ground_z,
                         const std::vector<float> &precip_depth,
                         const Params &p);

// Step 2: percolation (vertical drainage) using K(theta) ~ theta^3 between
// field capacity and wilting point. Water draining out of the bottom of the
// map recharges `groundwater`. Correctly "free-falls" through any
// underground AIR voids (caves) so it lands on the first solid floor below.
void soil_percolation_step(Game_map &map, const std::vector<int> &ground_z,
                           const std::vector<float> &soil_variation,
                           const Params &p, GroundwaterGrid &groundwater);

// Step 3: capillary rise - small upward diffusive exchange between vertically
// adjacent tiles when the lower one is wetter than the upper one's field
// capacity. Skips pairs separated by AIR (no capillary connection).
void capillary_rise_step(Game_map &map, const std::vector<int> &ground_z,
                         const std::vector<float> &soil_variation,
                         const Params &p);

// Step 4: D8 (8-direction) steepest-descent overland flow for surface ponds.
// Water flows toward the lowest neighbouring (ground + pond) elevation; flow
// off the map edge is accumulated into `runoff_to_ocean` (informational).
void overland_flow_step(Game_map &map, const std::vector<int> &ground_z,
                        const Params &p, float &runoff_to_ocean);

// Step 5: evapotranspiration. Surface tiles (and ponds) lose moisture
// proportional to wind speed * vapor-pressure-deficit * surface wetness;
// the evaporated water is returned to `climate`'s vapor field at that
// column. Returns total evaporated volume (diagnostic).
float evapotranspiration_step(Game_map &map, ClimateSystem &climate,
                              const std::vector<int> &ground_z,
                              const Params &p);

} // namespace hydro