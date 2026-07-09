#include "compositor.hpp"
#include <cassert>

Compositor::Compositor(int w, int h) : width_(w), height_(h) {}

void Compositor::composite(const std::vector<const RenderPlane*>& planes,
                           RenderPlane& output) {
    // Ensure output dimensions match – crash early if misused.
    assert(output.width() == width_ && output.height() == height_);

    // Fill with opaque black.
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            output.set(x, y, ' ', 7, 0);

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