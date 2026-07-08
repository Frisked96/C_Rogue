#include "render_backend.hpp"
#include <iostream>

RenderBackend::RenderBackend(int w, int h)
    : width_(w), height_(h), front_buffer_(w, h), needs_full_redraw_(true) {}

void RenderBackend::present(const RenderPlane &frame) {
  std::string output;
  output.reserve(static_cast<size_t>(width_) * height_ * 4);

  int cur_fg = -1;
  int cur_bg = -1;

  if (needs_full_redraw_) {
    // -----------------------------------------------------------
    // Full redraw: sequential row-by-row output (first frame, or
    // after invalidate() is called).
    // -----------------------------------------------------------
    output += "\033[1;1H"; // cursor to top-left
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        const Cell &cell = frame.get(x, y);
        emit_cell(output, cell, cur_fg, cur_bg);
      }
      output += "\033[0m\n";
      cur_fg = -1;
      cur_bg = -1;
    }
    needs_full_redraw_ = false;
  } else {
    // -----------------------------------------------------------
    // Diff-based update: only emit cells that changed since the
    // last present() call.
    // -----------------------------------------------------------
    for (int y = 0; y < height_; ++y) {
      bool cursor_placed = false;
      for (int x = 0; x < width_; ++x) {
        const Cell &new_cell = frame.get(x, y);
        const Cell &old_cell = front_buffer_.get(x, y);
        if (new_cell != old_cell) {
          if (!cursor_placed) {
            // Move cursor to (row, col) — 1-indexed
            output += "\033[" + std::to_string(y + 1) + ";" +
                      std::to_string(x + 1) + "H";
            cur_fg = -1;
            cur_bg = -1;
            cursor_placed = true;
          } else {
            // If the previous cell on this row was also dirty we can
            // skip the cursor-move and just keep printing.  But if
            // there was a gap of clean cells we need to reposition.
            // For simplicity we always reposition; a future
            // optimisation could track runs of dirty cells.
            output += "\033[" + std::to_string(y + 1) + ";" +
                      std::to_string(x + 1) + "H";
            cur_fg = -1;
            cur_bg = -1;
          }
          emit_cell(output, new_cell, cur_fg, cur_bg);
        }
      }
    }
  }

  if (!output.empty()) {
    std::cout << output << std::flush;
  }

  // Copy the frame into the front buffer so the next call can diff
  // against it.
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      front_buffer_.get(x, y) = frame.get(x, y);
    }
  }
}

void RenderBackend::invalidate() { needs_full_redraw_ = true; }

void RenderBackend::move_cursor_below() {
  std::cout << "\033[" << (height_ + 1) << ";1H";
}

void RenderBackend::emit_cell(std::string &out, const Cell &cell, int &cur_fg,
                              int &cur_bg) {
  char glyph = cell.glyph;
  if (glyph == '\0')
    glyph = ' ';

  if (cell.fg != cur_fg) {
    out += "\033[38;5;" + std::to_string(cell.fg) + "m";
    cur_fg = cell.fg;
  }
  if (cell.bg != cur_bg) {
    if (cell.bg == 0) {
      out += "\033[49m"; // default background
    } else {
      out += "\033[48;5;" + std::to_string(cell.bg) + "m";
    }
    cur_bg = cell.bg;
  }
  out += glyph;
}
