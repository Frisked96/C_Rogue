

#include "engine.hpp"
#include "map_gen/visibility.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>

Engine::Engine(int width, int height) : is_running(true) {



  // Initialize map with a random seed
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  int seed = std::rand();

  event_bus = std::make_unique<EventBus>();
  object_prototype_db = std::make_unique<ObjectPrototypeDB>();
  object_prototype_db->load_defaults();
  object_manager =
      std::make_unique<ObjectManager>(*object_prototype_db, *event_bus);

  map = std::make_unique<Game_map>(100, 100, 100); // 100x100x100 map
  map->generate(seed, object_prototype_db.get(), object_manager.get());

  renderer = std::make_unique<Renderer>(width, height);

  // Initialize input handler
  input_handler = std::make_unique<InputHandler>();

  // Find a valid spawn point for the player (start from top and go down until
  // we hit ground)
  int spawn_x = 50;
  int spawn_y = 50;
  auto is_blocked_for_spawn = [&](int cx, int cy, int cz) {
    if (map->get_tile(cx, cy, cz).mat().is_solid)
      return true;
    if (object_manager->spatial().has_any(cx, cy, cz)) {
      for (auto uid : object_manager->spatial().get_at(cx, cy, cz)) {
        auto *obj = object_manager->get(uid);
        if (obj &&
            object_manager->proto_db().get(obj->prototype_id).is_blocking)
          return true;
      }
    }
    return false;
  };

  int spawn_z = 99;
  while (spawn_z > 0 && !is_blocked_for_spawn(spawn_x, spawn_y, spawn_z)) {
    spawn_z--;
  }
  // Spawn 1 tile above ground (in the air)
  if (spawn_z < 99)
    spawn_z++;

  player_id =
      entityManager.spawn(EntityType::PLAYER, spawn_x, spawn_y, spawn_z);
  last_msg = "Welcome to C_Rogue! Explore the mountains.";

  // Perform an initial full screen clear
  std::cout << "\033[2J\033[1;1H";
}

Engine::~Engine() {}

void Engine::run() {
  while (is_running) {
    render();
    handle_input();
    // Update physiological systems
    entityManager.update(0.1f); // 100ms ticks
  }
}

void Engine::render() {
  Entity *player = entityManager.get(player_id);
  int player_x = 0;
  int player_y = 0;
  int player_z = 0;
  if (player) {
    player_x = player->state.x;
    player_y = player->state.y;
    player_z = player->state.z;
    Visibility::compute_fov(*map, player->state.x, player->state.y,
                            player->state.z, player->props().vision_radius);
  }

  renderer->render(*map, entityManager, object_manager.get(), player, player_z,
                   player_x, player_y, last_msg);
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
  case Action::TOGGLE_DEBUG:
    renderer->toggle_debug_mode();
    last_msg =
        renderer->is_debug_mode() ? "[DEBUG MODE ON]" : "[DEBUG MODE OFF]";
    break;
  default:
    break;
  }

  if (dx != 0 || dy != 0 || dz != 0) {
    Entity *player = entityManager.get(player_id);
    if (player) {
      int nx = player->state.x + dx;
      int ny = player->state.y + dy;
      int nz = player->state.z + dz;

      // Surface-following logic
      bool is_blocked_base = entityManager.is_blocked(nx, ny, nz, *map);
      if (!is_blocked_base && object_manager->spatial().has_any(nx, ny, nz)) {
        for (auto uid : object_manager->spatial().get_at(nx, ny, nz)) {
          auto *obj = object_manager->get(uid);
          if (obj &&
              object_manager->proto_db().get(obj->prototype_id).is_blocking) {
            is_blocked_base = true;
            break;
          }
        }
      }

      if (is_blocked_base) {
        // Attempt to climb (up to 2m)
        bool is_blocked_z1 = entityManager.is_blocked(nx, ny, nz + 1, *map);
        if (!is_blocked_z1 &&
            object_manager->spatial().has_any(nx, ny, nz + 1)) {
          for (auto uid : object_manager->spatial().get_at(nx, ny, nz + 1)) {
            auto *obj = object_manager->get(uid);
            if (obj &&
                object_manager->proto_db().get(obj->prototype_id).is_blocking) {
              is_blocked_z1 = true;
              break;
            }
          }
        }
        if (!is_blocked_z1) {
          nz++;
          last_msg = "You climb up.";
        } else {
          bool is_blocked_z2 = entityManager.is_blocked(nx, ny, nz + 2, *map);
          if (!is_blocked_z2 &&
              object_manager->spatial().has_any(nx, ny, nz + 2)) {
            for (auto uid : object_manager->spatial().get_at(nx, ny, nz + 2)) {
              auto *obj = object_manager->get(uid);
              if (obj && object_manager->proto_db()
                             .get(obj->prototype_id)
                             .is_blocking) {
                is_blocked_z2 = true;
                break;
              }
            }
          }
          if (!is_blocked_z2) {
            nz += 2;
            last_msg = "You scramble up the ridge.";
          } else {
            last_msg = "Blocked by " + map->get_tile(nx, ny, nz).mat().name +
                       " or object.";
            return;
          }
        }
      } else {
        // Gravity / Descending logic
        int start_z = nz;
        auto is_blocked_at = [&](int cx, int cy, int cz) {
          if (entityManager.is_blocked(cx, cy, cz, *map))
            return true;
          if (object_manager->spatial().has_any(cx, cy, cz)) {
            for (auto uid : object_manager->spatial().get_at(cx, cy, cz)) {
              auto *obj = object_manager->get(uid);
              if (obj &&
                  object_manager->proto_db().get(obj->prototype_id).is_blocking)
                return true;
            }
          }
          return false;
        };

        while (nz > 0 && !is_blocked_at(nx, ny, nz) &&
               !is_blocked_at(nx, ny, nz - 1)) {

          // Buoyancy: Stop falling if we hit deep enough water
          const Tile &current_tile = map->get_tile(nx, ny, nz);
          if (current_tile.material == MaterialType::AIR &&
              current_tile.state.liquid_volume >= 0.4f) {
            break;
          }
          nz--;
        }

        if (nz < start_z) {
          last_msg = (start_z - nz > 1) ? "You scramble down." : "You descend.";
        } else if (map->get_tile(nx, ny, nz).material == MaterialType::AIR &&
                   map->get_tile(nx, ny, nz).state.liquid_volume > 0.0f) {
          float m = map->get_tile(nx, ny, nz).state.liquid_volume;
          if (m >= 0.8f)
            last_msg = "You are swimming.";
          else if (m >= 0.4f)
            last_msg = "You wade through waist-deep water.";
          else
            last_msg = "You splash through ankle-deep water.";
        } else {
          last_msg = "You move forward.";
        }
      }

      auto is_blocked_at_final = [&](int cx, int cy, int cz) {
        if (entityManager.is_blocked(cx, cy, cz, *map))
          return true;
        if (object_manager->spatial().has_any(cx, cy, cz)) {
          for (auto uid : object_manager->spatial().get_at(cx, cy, cz)) {
            auto *obj = object_manager->get(uid);
            if (obj &&
                object_manager->proto_db().get(obj->prototype_id).is_blocking)
              return true;
          }
        }
        return false;
      };

      if (!is_blocked_at_final(nx, ny, nz)) {
        entityManager.get_spatial_grid().move(player_id, player->state.x,
                                              player->state.y, player->state.z,
                                              nx, ny, nz);
        player->state.x = nx;
        player->state.y = ny;
        player->state.z = nz;
      }
    }
  }
}
