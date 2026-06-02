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
    : entityFactory(entityManager), is_running(true), screen_width(width),
      screen_height(height) {

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
  map = std::make_unique<Game_map>(width, height);
  map->generate();

  // Initialize renderer
  renderer = std::make_unique<Terminal_renderer>(width, height);

  // Initialize input handler
  input_handler = std::make_unique<InputHandler>();

  // Initialize player at center
  player = entityFactory.createPlayer(width / 2, height / 2, "Player", '@');

  // Perform an initial full screen clear
  std::cout << "\033[2J\033[1;1H";
}

Engine::~Engine() {
  // Show cursor again and reset color/formatting
  std::cout << "\033[?25h\033[0m\n";
}

void Engine::run() {
  while (is_running) {
    render();
    handle_input();
  }
}

void Engine::render() {
  // Move cursor to top-left and hide cursor
  std::cout << "\033[H\033[?25l";

  // 1. Draw map
  for (int y = 0; y < map->get_height(); ++y) {
    for (int x = 0; x < map->get_width(); ++x) {
      Tile t = map->get_tile(x, y);
      renderer->set_tile(x, y, t.glyph);
    }
  }

  // 2. Draw player (over map)
  if (player->hasComponent<PositionComponent>() &&
      player->hasComponent<RenderComponent>()) {
    auto pos = player->getComponent<PositionComponent>();
    auto render = player->getComponent<RenderComponent>();
    renderer->set_tile(pos->x, pos->y, render->glyph);
  }

  // 3. Render to screen
  renderer->draw();

  // Print stats/help (pad with spaces to overwrite old text)
  if (player->hasComponent<PositionComponent>()) {
    auto pos = player->getComponent<PositionComponent>();
    std::cout << "Player: (" << pos->x << ", " << pos->y << ")               \n";
  }
  std::cout << "Controls: WASD to move, Q to quit.                         \n";
}

void Engine::handle_input() {
  std::cout << ">       \b\b\b\b\b\b"; // Print prompt and space for input, move back
  char input;
  std::cin >> input;
  
  // Clear the rest of the line after input
  std::cout << "\033[K";

  Action action = input_handler->process_input(input);

  int dx = 0;
  int dy = 0;

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
  case Action::QUIT:
    is_running = false;
    break;
  default:
    break;
  }

  if (dx != 0 || dy != 0) {
    if (player->hasComponent<PositionComponent>()) {
      auto pos = player->getComponent<PositionComponent>();
      int new_x = pos->x + dx;
      int new_y = pos->y + dy;

      if (map->can_walk(new_x, new_y)) {
        pos->x = new_x;
        pos->y = new_y;
      }
    }
  }
}
