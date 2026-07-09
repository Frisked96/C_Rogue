#include "compositor.hpp"
#include <cassert>

Compositor::Compositor(int w, int h) : width_(w), height_(h) {}

void Compositor::composite(const std::vector<const RenderPlane*>& planes,
                           RenderPlane& output) {
    assert(output.width()  == width_ && "output plane width mismatch");
    assert(output.height() == height_ && "output plane height mismatch");

    // Start with a fully opaque black background.
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            output.get(x, y) = { U' ', 7, 0, false };   // opaque space

    // Overlay non‑transparent cells from bottom to top.
    for (const auto* plane : planes) {
        if (!plane) continue;
        for (int y = 0; y < height_; ++y)
            for (int x = 0; x < width_; ++x) {
                const Cell& cell = plane->get(x, y);
                if (!cell.is_transparent())
                    output.get(x, y) = cell;
            }
    }
}