#pragma once

#include "render_types.hpp"
#include <string>

// Double-buffered ANSI terminal backend.
//
// Maintains a front buffer (what the terminal currently shows) and accepts a
// completed frame via present().  Only the cells that differ between the new
// frame and the front buffer are emitted as ANSI escape sequences, eliminating
// full-screen redraws and terminal flicker.
class RenderBackend {
public:
  RenderBackend(int w, int h);

  // Push a completed frame to the terminal.  Diffs against the previous frame
  // and outputs only changed cells.
  void present(const RenderPlane &frame);

  // Force the next present() to redraw every cell (e.g. after a terminal
  // resize or mode switch).
  void invalidate();

  // Position the cursor on the line immediately below the rendered grid so
  // that UI text can be printed without overwriting the map.
  void move_cursor_below();

  int width() const noexcept { return width_; }
  int height() const noexcept { return height_; }

private:
  int width_;
  int height_;
  RenderPlane front_buffer_; // mirrors what is currently on the terminal
  bool needs_full_redraw_;

  // Append the ANSI codes + character for a single cell to `out`.
  // `cur_fg` / `cur_bg` are tracked so redundant color changes are skipped.
  void emit_cell(std::string &out, const Cell &cell, int &cur_fg, int &cur_bg);
};
