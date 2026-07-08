#pragma once

#include "Entity/entity_manager.hpp"
#include "Object/event_bus.hpp"
#include "Object/object_manager.hpp"
#include "Object/object_prototype_db.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "renderer/renderer.hpp"
#include <memory>

class Engine {
private:
  std::unique_ptr<EventBus> event_bus;
  std::unique_ptr<ObjectPrototypeDB> object_prototype_db;
  std::unique_ptr<ObjectManager> object_manager;
  std::unique_ptr<Game_map> map;
  std::unique_ptr<Renderer> renderer;
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
