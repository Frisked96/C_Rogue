#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include "engine.hpp"
#include "map_gen/visibility.hpp"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <algorithm>

Engine::Engine(int width, int height)
    : is_running(true) {

#ifdef _WIN32
  // Enable ANSI escape codes on Windows
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE) {
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(hOut, dwMode);
    }
  }
#endif

  // Initialize map with a random seed
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  int seed = std::rand();
  map = std::make_unique<Game_map>(100, 100, 100); // 100x100x100 map
  map->generate(seed);

  // Initialize renderer
  renderer = std::make_unique<Terminal_renderer>(width, height);

  // Initialize input handler
  input_handler = std::make_unique<InputHandler>();

  // Find a valid spawn point for the player (start from top and go down until we hit ground)
  int spawn_x = 50;
  int spawn_y = 50;
  int spawn_z = 99;
  while (spawn_z > 0 && map->get_tile(spawn_x, spawn_y, spawn_z).material == MaterialType::AIR) {
      spawn_z--;
  }
  // Spawn 1 tile above ground (in the air)
  if (spawn_z < 99) spawn_z++;

  player_id = entityManager.spawn(EntityType::PLAYER, spawn_x, spawn_y, spawn_z);
  last_msg = "Welcome to C_Rogue! Explore the mountains.";

  // Perform an initial full screen clear
  std::cout << "\033[2J\033[1;1H";
}

Engine::~Engine() {
  // Show cursor again and reset color/formatting
  std::cout << "\033[?25h\033[0m\n" << std::flush;
}

void Engine::run() {
  while (is_running) {
    render();
    handle_input();
    // Update physiological systems
    entityManager.update(0.1f); // 100ms ticks
  }
}

void Engine::render() {
  renderer->clear_screen();
  renderer->clear_buffer();

  int player_x = 0;
  int player_y = 0;
  int player_z = 0;
  Entity* player = entityManager.get(player_id);
  if (player) {
    player_x = player->state.x;
    player_y = player->state.y;
    player_z = player->state.z;
    Visibility::compute_fov(*map, player->state.x, player->state.y, player->state.z, player->props().vision_radius);
  }

  renderer->render_map(*map, player_z, player_x, player_y);
  renderer->render_entities(entityManager, *map, player_z, player_x, player_y);
  
  renderer->draw();
  renderer->draw_ui(player, *map, last_msg);
}

void Engine::handle_input() {
  Action action = input_handler->get_action();

  int dx = 0;
  int dy = 0;
  int dz = 0;

  switch (action) {
  case Action::MOVE_UP:
    dy = -1;
    break;
  case Action::MOVE_DOWN:
    dy = 1;
    break;
  case Action::MOVE_LEFT:
    dx = -1;
    break;
  case Action::MOVE_RIGHT:
    dx = 1;
    break;
  case Action::MOVE_LEVEL_UP:
    dz = 1;
    break;
  case Action::MOVE_LEVEL_DOWN:
    dz = -1;
    break;
  case Action::QUIT:
    is_running = false;
    break;
  default:
    break;
  }

  if (dx != 0 || dy != 0 || dz != 0) {
    Entity* player = entityManager.get(player_id);
    if (player) {
      int nx = player->state.x + dx;
      int ny = player->state.y + dy;
      int nz = player->state.z + dz;

      // Surface-following logic
      if (entityManager.get_spatial_grid().is_blocked(nx, ny, nz, *map)) {
          // Attempt to climb (up to 2m)
          if (!entityManager.get_spatial_grid().is_blocked(nx, ny, nz + 1, *map)) {
              nz++;
              last_msg = "You climb up.";
          } else if (!entityManager.get_spatial_grid().is_blocked(nx, ny, nz + 2, *map)) {
              nz += 2;
              last_msg = "You scramble up the ridge.";
          } else {
              last_msg = "Blocked by " + map->get_tile(nx, ny, nz).mat().name + ".";
              return;
          }
      } else {
          // Gravity / Descending logic
          int start_z = nz;
          while (nz > 0 && 
                 !entityManager.get_spatial_grid().is_blocked(nx, ny, nz, *map) &&
                 !entityManager.get_spatial_grid().is_blocked(nx, ny, nz - 1, *map)) {
              
              // Buoyancy: Stop falling if we hit deep enough water
              const Tile& current_tile = map->get_tile(nx, ny, nz);
              if (current_tile.material == MaterialType::WATER_FRESH && current_tile.state.moisture >= 0.4f) {
                  break;
              }
              nz--;
          }
          
          if (nz < start_z) {
              last_msg = (start_z - nz > 1) ? "You scramble down." : "You descend.";
          } else if (map->get_tile(nx, ny, nz).material == MaterialType::WATER_FRESH) {
              float m = map->get_tile(nx, ny, nz).state.moisture;
              if (m >= 0.8f) last_msg = "You are swimming.";
              else if (m >= 0.4f) last_msg = "You wade through waist-deep water.";
              else last_msg = "You splash through ankle-deep water.";
          } else {
              last_msg = "You move forward.";
          }
      }

      if (!entityManager.get_spatial_grid().is_blocked(nx, ny, nz, *map)) {
        entityManager.get_spatial_grid().move(player_id, player->state.x, player->state.y, player->state.z, nx, ny, nz);
        player->state.x = nx;
        player->state.y = ny;
        player->state.z = nz;
      }
    }
  }
}
