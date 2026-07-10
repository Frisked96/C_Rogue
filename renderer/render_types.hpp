#pragma once

#include <string>   // for char32_t (C++11 built‑in, but header for char_traits)
#include <vector>

// A single character cell on the terminal grid.
struct Cell {
    char32_t glyph      = U' ';   // the codepoint to display
    int      fg         = 7;      // foreground colour (0‑255)
    int      bg         = 0;      // background colour (0‑255)
    bool     transparent = true;  // if true, this cell is invisible

    bool is_transparent() const noexcept { return transparent; }

    bool operator==(const Cell& other) const noexcept {
        return glyph == other.glyph &&
               fg == other.fg &&
               bg == other.bg &&
               transparent == other.transparent;
    }
    bool operator!=(const Cell& other) const noexcept { return !(*this == other); }
};

// A 2D grid of Cells representing one rendering layer.
// Multiple RenderPlanes are composited together (bottom‑to‑top) by the
// Compositor to produce a final frame.
class RenderPlane {
public:
    RenderPlane() : width_(0), height_(0) {}
    RenderPlane(int w, int h)
        : width_(w), height_(h),
          cells_(static_cast<size_t>(w) * static_cast<size_t>(h)) {}

    void resize(int w, int h) {
        width_ = w;
        height_ = h;
        cells_.assign(static_cast<size_t>(w) * static_cast<size_t>(h), Cell{});
    }

    // Reset every cell to transparent with default colours.
    void clear() {
        for (auto& cell : cells_)
            cell = {};   // default‑constructed Cell: transparent, U' ', fg=7, bg=0
    }

    // Set a cell. Non‑transparent by default (transparent = false).
    void set(int x, int y, char32_t glyph, int fg = 7, int bg = 0) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_)
            cells_[static_cast<size_t>(y) * width_ + x] =
                { glyph, fg, bg, false };  // explicit transparency flag = false
    }

    const Cell& get(int x, int y) const { return cells_[static_cast<size_t>(y) * width_ + x]; }
    Cell&       get(int x, int y)       { return cells_[static_cast<size_t>(y) * width_ + x]; }

    int width()  const noexcept { return width_; }
    int height() const noexcept { return height_; }

private:
    int width_, height_;
    std::vector<Cell> cells_;
};