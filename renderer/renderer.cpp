#include "renderer.hpp"

Renderer::Renderer(int w, int h)
    : backend_(w, h), compositor_(w, h), map_plane_(w, h), entity_plane_(w, h),
      composed_(w, h) {}

void Renderer::render(const Game_map &map, EntityManager &entityManager,
                      ObjectManager *objManager, const Entity *player, int z,
                      int cam_x, int cam_y, const std::string &msg) {
  // --- 1. Clear the layer planes ---
  map_plane_.clear();
  entity_plane_.clear();

  // --- 2. Populate the map layer ---
  if (debug_mode_) {
    world_renderer_.render_debug(map_plane_, map, z, cam_x, cam_y);
  } else {
    world_renderer_.render(map_plane_, map, z, cam_x, cam_y);
  }

  // --- 3. Populate the entity layer ---
  entity_renderer_.render(entity_plane_, entityManager, objManager, map, z,
                          cam_x, cam_y);

  // --- 4. Composite all layers (bottom-to-top) ---
  std::vector<const RenderPlane *> layers = {&map_plane_, &entity_plane_};
  compositor_.composite(layers, composed_);

  // --- 5. Push the composed frame to the terminal ---
  backend_.present(composed_);

  // --- 6. Draw UI text below the grid ---
  backend_.move_cursor_below();
  ui_renderer_.render(player, objManager, map, msg, debug_mode_);
}

void Renderer::toggle_debug_mode() {
  debug_mode_ = !debug_mode_;
  // Force a full redraw when switching modes because every cell changes.
  backend_.invalidate();
}

bool Renderer::is_debug_mode() const { return debug_mode_; }
