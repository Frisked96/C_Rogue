#pragma once

#include "Entity/player.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "renderer.hpp"
#include <memory>

class Engine {
private:
  std::unique_ptr<Game_map> map;
  std::unique_ptr<Terminal_renderer> renderer;
  std::unique_ptr<InputHandler> input_handler;
  std::unique_ptr<Player> player;

  bool is_running;
  int screen_width;
  int screen_height;

  void handle_input();
  void render();

public:
  Engine(int width, int height);
  void run();
};
