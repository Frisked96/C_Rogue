#pragma once

// =============================================================================
// simulator.hpp
//
// MapSimulator orchestrates the layered climate/hydrology system declared in
// climate_hydrology.hpp. The public interface is unchanged from the original
// simulator (`run(Game_map&, int seed)`), so this should drop in as a
// replacement without touching call sites.
//
// `balance_basins` is kept (it was referenced elsewhere per the original
// code's comment) but is now a real diagnostic: it returns the total volume
// of ponded surface water (lakes/rivers/oceans) across the map, in
// metres-equivalent.
// =============================================================================

#include "climate_hydrology.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class Game_map; // forward declaration only; full definition needed in the .cpp

class MapSimulator {
public:
  // `num_years` defaults to 65 to match the original simulation length and
  // preserve the original call signature `run(game_map, seed)`. Useful for
  // the demo harness (and for tuning) to verify longer-term equilibrium.
  void run(Game_map &game_map, int seed, int num_years = 65);

  // Diagnostic: total ponded surface-water volume (m^3-equivalent).
  float balance_basins(Game_map &game_map);

  // Additional diagnostics, useful for debugging/UI/tuning:
  float total_runoff_to_ocean() const { return total_runoff_to_ocean_; }
  float total_atmospheric_vapor() const { return climate_.total_vapor(); }
  float total_groundwater() const { return groundwater_.total_table(); }

  const std::unordered_map<std::string, double> &get_timings() const {
    return step_timings_;
  }

private:
  void initialize(Game_map &game_map, int seed);
  void simulate_substep(Game_map &game_map, int substep, int year);

  hydro::Params params_;
  NoiseGen noise_;

  std::vector<int> ground_z_;
  std::vector<float> soil_variation_;

  hydro::ClimateSystem climate_;
  hydro::GroundwaterGrid groundwater_;
  hydro::OverlandFlowBuffers overland_bufs_;

  int width_ = 0, height_ = 0, depth_ = 0;
  bool initialized_ = false;

  float total_runoff_to_ocean_ = 0.0f; // informational accumulator

  std::unordered_map<std::string, double> step_timings_;
};
