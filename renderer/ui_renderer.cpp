#include "ui_renderer.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

void UIRenderer::render(const Entity* player, ObjectManager* objManager,
                        const Game_map& map, const std::string& msg,
                        bool debug_mode) {
  if (debug_mode) {
    render_debug(player, map, msg);
    return;
  }
  render_normal(player, objManager, map, msg);
}

void UIRenderer::render_normal(const Entity* player, ObjectManager* objManager,
                               const Game_map& map, const std::string& msg) {
  // Print stats/help (pad with spaces to overwrite old text)
  if (player) {
    int px = player->state.x;
    int py = player->state.y;
    int pz = player->state.z;

    std::string surface_name = "Air";
    if (pz > 0) {
      if (objManager && objManager->spatial().has_any(px, py, pz - 1)) {
        for (auto uid : objManager->spatial().get_at(px, py, pz - 1)) {
          auto* obj = objManager->get(uid);
          if (obj && objManager->proto_db().get(obj->prototype_id).is_blocking) {
            surface_name = objManager->proto_db().get(obj->prototype_id).name;
            break;
          }
        }
      }
      if (surface_name == "Air") {
        surface_name = map.get_tile(px, py, pz - 1).mat().name;
      }
    }

    bool ceiling = (pz < map.get_depth() - 1) &&
                   map.get_tile(px, py, pz + 1).material != MaterialType::AIR;

    std::cout << "\033[1;37m" << player->props().name << " | Alt: " << pz << "m"
              << " | Standing on: " << surface_name << "          \n";

    std::cout << "Hunger: " << (int)(player->state.hunger * 100) << "% "
              << "| Thirst: " << (int)(player->state.thirst * 100) << "% "
              << "| Site: "
              << (ceiling ? "\033[33mUnderground\033[37m"
                          : "\033[36mOpen Sky\033[37m")
              << "    \n";

    // Message Log
    std::cout << "\033[1;33mLog: " << msg
              << "\033[0m                                          \n";

    // Simple cardinal surroundings (Compass)
    auto get_alt_diff = [&](int dx, int dy) -> std::string {
      int nx = px + dx;
      int ny = py + dy;
      if (!map.is_in_bounds(nx, ny, pz))
        return "???";
      if (!map.is_visible(nx, ny, pz))
        return "???"; // Hide unknown altitude
      int nz = pz;
      // Find surface at nx, ny
      while (nz > 0 && map.get_tile(nx, ny, nz).material == MaterialType::AIR)
        nz--;
      int diff = nz - (pz - 1); // diff from ground under player
      if (diff == 0)
        return "=";
      return (diff > 0 ? "+" : "") + std::to_string(diff);
    };

    std::cout << "Near: [N:" << get_alt_diff(0, -1)
              << "] [S:" << get_alt_diff(0, 1) << "] [W:" << get_alt_diff(-1, 0)
              << "] [E:" << get_alt_diff(1, 0) << "]    \n";
  }
  std::cout << "\033[0;32m-----------------------------------------------------"
               "---\033[0m              \n";
}

void UIRenderer::render_debug(const Entity* player, const Game_map& map,
                              const std::string& msg) {
  if (!player) {
    std::cout << "\033[1;31m[DEBUG] No player entity\033[0m\n";
    return;
  }

  int px = player->state.x;
  int py = player->state.y;
  int pz = player->state.z;

  // Find surface_z below player
  int surface_z = pz;
  while (surface_z > 0 &&
         map.get_tile(px, py, surface_z).material == MaterialType::AIR) {
    surface_z--;
  }

  // --- Line 1: Header ---
  std::cout << "\033[1;32m[DEBUG MODE] Pos: (" << px << ", " << py << ", " << pz
            << ") | Surface Elev: " << surface_z << "m\033[0m    \n";

  // Helper to format floats
  auto ff = [](float v) -> std::string {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
  };

  // --- Lines 2-4: Tile player is INSIDE (z-level) ---
  if (map.is_in_bounds(px, py, pz)) {
    const Tile &t = map.get_tile(px, py, pz);
    const auto &m = t.mat();
    std::cout << "\033[36mINSIDE [" << pz << "]: \033[37m" << m.name
              << " | glyph:'" << m.glyph
              << "' solid:" << (m.is_solid ? "Y" : "N")
              << " opaque:" << (m.is_opaque ? "Y" : "N") << "    \n";
    std::cout << "\033[36m  Water: \033[37m" << ff(t.state.liquid_volume)
              << " \033[36mIce: \033[37m" << ff(t.state.frozen_volume)
              << " \033[36mTemp: \033[37m" << ff(t.state.temperature)
              << "K \033[36mCompact: \033[37m" << ff(t.state.compaction)
              << "    \n";
    std::cout << "\033[36m  Density: \033[37m" << ff(m.density_kgm3)
              << "kg/m3 \033[36mPoros: \033[37m" << ff(m.max_porosity)
              << " \033[36mPerm: \033[37m" << ff(m.permeability)
              << " \033[36mShear: \033[37m" << ff(m.shear_strength)
              << "kPa    \n";
  }

  // --- Lines 5-7: Tile player is STANDING ON (z-1) ---
  if (pz > 0 && map.is_in_bounds(px, py, pz - 1)) {
    const Tile &t = map.get_tile(px, py, pz - 1);
    const auto &m = t.mat();
    std::cout << "\033[36mON [" << (pz - 1) << "]: \033[37m" << m.name
              << " | glyph:'" << m.glyph
              << "' solid:" << (m.is_solid ? "Y" : "N")
              << " opaque:" << (m.is_opaque ? "Y" : "N") << "    \n";
    std::cout << "\033[36m  Water: \033[37m" << ff(t.state.liquid_volume)
              << " \033[36mIce: \033[37m" << ff(t.state.frozen_volume)
              << " \033[36mTemp: \033[37m" << ff(t.state.temperature)
              << "K \033[36mCompact: \033[37m" << ff(t.state.compaction)
              << "    \n";
    std::cout << "\033[36m  Density: \033[37m" << ff(m.density_kgm3)
              << "kg/m3 \033[36mPoros: \033[37m" << ff(m.max_porosity)
              << " \033[36mPerm: \033[37m" << ff(m.permeability)
              << " \033[36mShear: \033[37m" << ff(m.shear_strength)
              << "kPa    \n";
  }

  // --- Water Compass ---
  // Scans each cardinal direction. At each step, checks the full
  // vertical column from pz downward to find water at ANY z-level.
  static const int dx[] = {0, 0, -1, 1};
  static const int dy[] = {-1, 1, 0, 0};
  static const char *dir_names[] = {"N", "S", "W", "E"};
  int water_dist[4] = {-1, -1, -1, -1};
  int water_z[4] = {-1, -1, -1, -1};

  for (int d = 0; d < 4; ++d) {
    for (int step = 1; step <= 50; ++step) {
      int nx = px + dx[d] * step;
      int ny = py + dy[d] * step;
      bool found = false;
      // Scan from pz downward through air to find water
      for (int sz = pz; sz >= 0; --sz) {
        if (!map.is_in_bounds(nx, ny, sz))
          break;
        const Tile &nt = map.get_tile(nx, ny, sz);
        if (nt.material != MaterialType::AIR)
          break; // hit solid, stop
        if (nt.state.liquid_volume > 0.0f) {
          water_dist[d] = step;
          water_z[d] = sz;
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
  }

  // Check if player is currently in water (any z at player column)
  bool in_water = false;
  int player_water_z = -1;
  for (int sz = pz; sz >= 0; --sz) {
    if (!map.is_in_bounds(px, py, sz))
      break;
    const Tile &pt = map.get_tile(px, py, sz);
    if (pt.material != MaterialType::AIR)
      break;
    if (pt.state.liquid_volume > 0.0f) {
      in_water = true;
      player_water_z = sz;
      break;
    }
  }

  std::cout << "\033[33m";
  if (in_water) {
    std::cout << "[IN WATER z:" << player_water_z << "] ";
  }
  std::cout << "Water:";
  for (int d = 0; d < 4; ++d) {
    std::cout << " [" << dir_names[d] << ":";
    if (water_dist[d] >= 0)
      std::cout << water_dist[d] << "@z" << water_z[d];
    else
      std::cout << "---";
    std::cout << "]";
  }

  // Find nearest
  int nearest = -1;
  int nearest_dist = 51;
  for (int d = 0; d < 4; ++d) {
    if (water_dist[d] >= 0 && water_dist[d] < nearest_dist) {
      nearest_dist = water_dist[d];
      nearest = d;
    }
  }
  std::cout << "  Nearest: " << (nearest >= 0 ? dir_names[nearest] : "NONE")
            << "\033[0m    \n";

  // --- Log ---
  std::cout << "\033[1;33mLog: " << msg
            << "\033[0m                                          \n";

  // --- Separator ---
  std::cout << "\033[0;32m[DEBUG]----------------------------------------------"
               "--------"
               "\033[0m    \n";
}
