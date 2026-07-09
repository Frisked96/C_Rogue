#include "compositor.hpp"

Compositor::Compositor(int w, int h) : width_(w), height_(h) {}

void Compositor::composite(const std::vector<const RenderPlane*>& planes,
                           RenderPlane& output) {
    // Fill with an opaque black background so every cell is defined.
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            output.set(x, y, ' ', 7, 0);

    // Layer planes bottom‑to‑top. Non‑transparent cells overwrite those below.
    for (const auto* plane : planes) {
        if (!plane) continue;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const Cell& cell = plane->get(x, y);
                if (!cell.is_transparent())
                    output.get(x, y) = cell;
            }
        }
    }
}