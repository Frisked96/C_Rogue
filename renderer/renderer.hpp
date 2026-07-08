#pragma once

#include "compositor.hpp"
#include "entity_renderer.hpp"
#include "render_backend.hpp"
#include "render_types.hpp"
#include "ui_renderer.hpp"
#include "world_renderer.hpp"

// Top-level renderer that orchestrates the layered pipeline:
//   1. WorldRenderer  -> map_plane_     (terrain, water, elevation)
//   2. EntityRenderer  -> entity_plane_  (creatures, objects)
//   3. Compositor      -> composed_      (flattened frame)
//   4. RenderBackend   -> terminal       (diff-based ANSI output)
//   5. UIRenderer      -> stdout         (HUD text below the grid)
class Renderer {
public:
  Renderer(int w, int h);

  // Execute the full render pipeline for one frame.
  void render(const Game_map &map, EntityManager &entityManager,
              ObjectManager *objManager, const Entity *player, int z,
              int cam_x, int cam_y, const std::string &msg);

  void toggle_debug_mode();
  bool is_debug_mode() const;

private:
  bool debug_mode_ = false;

  // Pipeline stages
  RenderBackend backend_;
  Compositor compositor_;
  WorldRenderer world_renderer_;
  EntityRenderer entity_renderer_;
  UIRenderer ui_renderer_;

  // Layer planes
  RenderPlane map_plane_;
  RenderPlane entity_plane_;
  RenderPlane composed_;
};
