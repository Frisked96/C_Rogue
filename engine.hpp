#pragma once

#include "Entity/entity_manager.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "renderer.hpp"
#include <memory>

class Engine {
private:
  std::unique_ptr<Game_map> map;
  std::unique_ptr<Terminal_renderer> renderer;
  std::unique_ptr<InputHandler> input_handler;

  // Simplified Entity System
  EntityManager entityManager;
  EntityID player_id;

  bool is_running;
  std::string last_msg;

  void handle_input();
  void render();

public:
  Engine(int width, int height);
  ~Engine();
  void run();
};
