#pragma once

#include "render_types.hpp"
#include <vector>

// Flattens multiple RenderPlanes (bottom‑to‑top) into a single output plane.
// Transparent cells are skipped, allowing lower layers to show through.
class Compositor {
public:
    Compositor(int w, int h);

    // Composite `planes` in order (index 0 = bottom‑most) into `output`.
    // `output` must have the same dimensions as the compositor.
    void composite(const std::vector<const RenderPlane*>& planes,
                   RenderPlane& output);

private:
    int width_, height_;
};