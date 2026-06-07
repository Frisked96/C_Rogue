#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include "engine.hpp"
#include "Entity/components.hpp"
#include <iostream>

Engine::Engine(int width, int height)
    : entityFactory(entityManager), is_running(true) {

  // Connect systems
  entityManager.setExternalListener(&systemManager);

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

  // Initialize map
  map = std::make_unique<Game_map>(width, height, 10); // 10 levels deep
  map->generate();

  // Initialize renderer
  renderer = std::make_unique<Terminal_renderer>(width, height);

  // Initialize input handler
  input_handler = std::make_unique<InputHandler>();

  // Initialize player at center, level 0
  player = entityFactory.createPlayer(width / 2, height / 2, 0, "Player", '@');

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
    // Update systems (anatomy, etc.)
    systemManager.update(entityManager);
  }
}

void Engine::render() {
  renderer->clear_screen();
  renderer->clear_buffer();

  int player_z = 0;
  if (player->hasComponent<PositionComponent>()) {
    player_z = player->getComponent<PositionComponent>()->z;
  }

  renderer->render_map(*map, player_z);
  renderer->render_entities(entityManager, player_z);
  
  renderer->draw();
  renderer->draw_ui(player);
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
    if (player->hasComponent<PositionComponent>()) {
      auto pos = player->getComponent<PositionComponent>();
      int new_x = pos->x + dx;
      int new_y = pos->y + dy;
      int new_z = pos->z + dz;

      // Use the new spatial grid for collision and support checks
      if (systemManager.getSpatialGrid().canMoveTo(player, new_x, new_y, new_z, *map)) {
        player->setPosition(new_x, new_y, new_z);
      }
    }
  }
}
