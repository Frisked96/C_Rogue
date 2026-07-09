#pragma once

#include "render_types.hpp"
#include <vector>

class Compositor {
public:
    Compositor(int w, int h);

    void composite(const std::vector<const RenderPlane*>& planes,
                   RenderPlane& output);

private:
    int width_, height_;
};