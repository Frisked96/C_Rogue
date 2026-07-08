#pragma once

#include <cstdint>
#include <vector>

// A single character cell on the terminal grid.
// glyph == '\0' signifies a transparent cell (nothing drawn at this position
// on this layer).
struct Cell {
  char glyph = '\0';
  int fg = 7;
  int bg = 0;

  bool is_transparent() const noexcept { return glyph == '\0'; }

  bool operator==(const Cell &other) const noexcept {
    return glyph == other.glyph && fg == other.fg && bg == other.bg;
  }
  bool operator!=(const Cell &other) const noexcept {
    return !(*this == other);
  }
};

// A 2D grid of Cells representing one rendering layer.
// Multiple RenderPlanes are composited together (bottom-to-top) by the
// Compositor to produce a final frame.
class RenderPlane {
public:
  RenderPlane() : width_(0), height_(0) {}
  RenderPlane(int w, int h)
      : width_(w), height_(h),
        cells_(static_cast<size_t>(h),
               std::vector<Cell>(static_cast<size_t>(w))) {}

  void resize(int w, int h) {
    width_ = w;
    height_ = h;
    cells_.assign(static_cast<size_t>(h),
                  std::vector<Cell>(static_cast<size_t>(w)));
  }

  // Reset every cell to transparent.
  void clear() {
    for (auto &row : cells_)
      for (auto &cell : row)
        cell = {'\0', 7, 0};
  }

  void set(int x, int y, char glyph, int fg = 7, int bg = 0) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
      cells_[y][x] = {glyph, fg, bg};
    }
  }

  const Cell &get(int x, int y) const { return cells_[y][x]; }
  Cell &get(int x, int y) { return cells_[y][x]; }

  int width() const noexcept { return width_; }
  int height() const noexcept { return height_; }

private:
  int width_;
  int height_;
  std::vector<std::vector<Cell>> cells_;
};
