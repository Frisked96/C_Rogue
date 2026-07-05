#include "simulator.hpp"
#include "game_map.hpp" // adjust this include if your Game_map header has a different name/path

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

// -----------------------------------------------------------------------
void MapSimulator::initialize(Game_map &game_map, int seed) {
  width_ = game_map.get_width();
  height_ = game_map.get_height();
  depth_ = game_map.get_depth();

  noise_.set_seed((uint32_t)seed);

  // Terrain doesn't change during the simulation, so the heightmap and
  // per-column soil-texture variation are computed once up front.
  ground_z_ = hydro::compute_ground_heightmap(game_map);
  soil_variation_ =
      hydro::compute_soil_variation(width_, height_, noise_, params_);

  climate_.init(width_, height_, params_);
  climate_.initialize(ground_z_, noise_);

  groundwater_.init(width_, height_, params_);
  groundwater_.initialize(ground_z_, noise_);

  total_runoff_to_ocean_ = 0.0f;
  initialized_ = true;

  step_timings_.clear();
  step_timings_["Atmosphere"] = 0.0;
  step_timings_["Infiltration"] = 0.0;
  step_timings_["Soil"] = 0.0;
  step_timings_["Surface"] = 0.0;
  step_timings_["Evapotranspiration"] = 0.0;
  step_timings_["Groundwater"] = 0.0;
}

// -----------------------------------------------------------------------
void MapSimulator::run(Game_map &game_map, int seed, int num_years) {
  initialize(game_map, seed);

  // Each "year" is broken into `substeps_per_year` daily-ish sub-steps so
  // no single step can move more than a tile's own capacity of water
  // (the Courant-safety requirement that the old *100.0f multipliers
  // violated). `num_years` defaults to 65, matching the original
  // simulation length; tune via hydro::Params::substeps_per_year and/or
  // this argument if you need a faster/slower world-gen pass.
  for (int year = 0; year < num_years; ++year) {
    for (int s = 0; s < params_.substeps_per_year; ++s) {
      simulate_substep(game_map, s, year);
    }
  }
}

// -----------------------------------------------------------------------
void MapSimulator::simulate_substep(Game_map &game_map, int substep, int year) {
  float season_phase =
      (float)substep / (float)params_.substeps_per_year; // 0..1 over the year
  float day_index = (float)(year * params_.substeps_per_year + substep);

  auto start = std::chrono::high_resolution_clock::now();

  // --- 1. Atmosphere ---
  if (substep % params_.wind_update_interval == 0) {
    climate_.update_wind(ground_z_, noise_, season_phase);
  }
  climate_.update_temperature(ground_z_, season_phase);
  climate_.advect();

  // Capture by const reference to avoid copying the internal buffer
  const std::vector<float> &precip =
      climate_.step_precipitation(noise_, day_index);

  auto end = std::chrono::high_resolution_clock::now();
  step_timings_["Atmosphere"] +=
      std::chrono::duration<double>(end - start).count();

  // --- 2. Infiltration ---
  start = std::chrono::high_resolution_clock::now();
  hydro::apply_precipitation(game_map, ground_z_, precip, params_);
  end = std::chrono::high_resolution_clock::now();
  step_timings_["Infiltration"] +=
      std::chrono::duration<double>(end - start).count();

  // --- 3. Soil column: percolation (bucket model) + capillary rise ---
  start = std::chrono::high_resolution_clock::now();
  hydro::soil_percolation_step(game_map, ground_z_, soil_variation_, params_,
                               groundwater_);
  hydro::capillary_rise_step(game_map, ground_z_, soil_variation_, params_);
  end = std::chrono::high_resolution_clock::now();
  step_timings_["Soil"] += std::chrono::duration<double>(end - start).count();

  // --- 4. Surface water: D8 overland flow ---
  start = std::chrono::high_resolution_clock::now();
  float runoff = 0.0f;
  hydro::overland_flow_step(game_map, ground_z_, params_, runoff,
                            overland_bufs_);
  total_runoff_to_ocean_ += runoff;
  end = std::chrono::high_resolution_clock::now();
  step_timings_["Surface"] +=
      std::chrono::duration<double>(end - start).count();

  // --- 5. Evapotranspiration (closes the loop back into the atmosphere) ---
  start = std::chrono::high_resolution_clock::now();
  hydro::evapotranspiration_step(game_map, climate_, ground_z_, params_);
  end = std::chrono::high_resolution_clock::now();
  step_timings_["Evapotranspiration"] +=
      std::chrono::duration<double>(end - start).count();

  // --- 6. Groundwater: slow diffusion + baseflow to surface ---
  start = std::chrono::high_resolution_clock::now();
  if (substep % params_.groundwater_update_interval == 0) {
    // Capture by const reference to avoid copying the internal buffer
    const std::vector<float> &discharge =
        groundwater_.update(ground_z_, game_map);
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        int i = y * width_ + x;
        if (discharge[i] > 0.0f) {
          hydro::add_surface_water(game_map, x, y, ground_z_[i], discharge[i],
                                   params_);
        }
      }
    }
  }
  end = std::chrono::high_resolution_clock::now();
  step_timings_["Groundwater"] +=
      std::chrono::duration<double>(end - start).count();
}

// -----------------------------------------------------------------------
float MapSimulator::balance_basins(Game_map &game_map) {
  if (!initialized_)
    return 0.0f;

  float total = 0.0f;
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      int i = y * width_ + x;
      int gz = ground_z_[i];
      if (gz + 1 < depth_) {
        const Tile &t = game_map.get_tile(x, y, gz + 1);
        if (t.material == MaterialType::AIR)
          total += t.state.liquid_volume;
      }
    }
  }
  return total;
}