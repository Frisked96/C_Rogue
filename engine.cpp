#include "engine.hpp"
#include "Entity/components.hpp"
#include <iostream>

Engine::Engine(int width, int height)
    : entityFactory(entityManager), is_running(true), screen_width(width),
      screen_height(height) {

  // Initialize map
  map = std::make_unique<Game_map>(width, height);
  map->generate();

  // Initialize renderer
  renderer = std::make_unique<Terminal_renderer>(width, height);

  // Initialize input handler
  input_handler = std::make_unique<InputHandler>();

  // Initialize player at center
  player = entityFactory.createPlayer(width / 2, height / 2, "Player", '@');
}

void Engine::run() {
  while (is_running) {
    render();
    handle_input();
  }
}

void Engine::render() {
  // Clear screen (ANSI escape code)
  std::cout << "\033[2J\033[1;1H";

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

  // Print stats/help
  if (player->hasComponent<PositionComponent>()) {
    auto pos = player->getComponent<PositionComponent>();
    std::cout << "Player: (" << pos->x << ", " << pos->y << ")\n";
  }
  std::cout << "Controls: WASD to move, Q to quit.\n";
}

void Engine::handle_input() {
  std::cout << "> ";
  char input;
  std::cin >> input;

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
