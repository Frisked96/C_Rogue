#include "render_backend.hpp"
#include <cstdio>
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int RenderBackend::instance_count_ = 0;

#ifdef _WIN32
static DWORD s_saved_mode_ = 0;
static UINT s_saved_cp_ = 0;
#endif

// ---------------------------------------------------------------------------
// Windows console initialisation / teardown
// ---------------------------------------------------------------------------
void RenderBackend::init_console() {
  if (instance_count_++ != 0)
    return; // already set up

#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE) {
    // Save original console mode, then enable virtual terminal processing
    GetConsoleMode(hOut, &s_saved_mode_);
    DWORD mode = s_saved_mode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
  }

  // Save original code page, then set output to UTF-8
  s_saved_cp_ = GetConsoleOutputCP();
  SetConsoleOutputCP(CP_UTF8);
#endif

  // Hide cursor during frame draws (will be shown again by move_cursor_below())
  std::cout << "\033[?25l";
}

void RenderBackend::restore_console() {
  if (--instance_count_ != 0)
    return;
  // Show cursor and reset attributes
  std::cout << "\033[?25h\033[0m" << std::flush;

#ifdef _WIN32
  // Restore original console mode and code page
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE) {
    SetConsoleMode(hOut, s_saved_mode_);
  }
  SetConsoleOutputCP(s_saved_cp_);
#endif
}

// ---------------------------------------------------------------------------
// UTF-8 encoding of a single codepoint (appends directly to buffer)
// ---------------------------------------------------------------------------
void RenderBackend::to_utf8(char32_t cp, std::string &out) {
  if (cp < 0x80) {
    out += static_cast<char>(cp);
  } else if (cp < 0x800) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x110000) {
    out += static_cast<char>(0xF0 | (cp >> 18));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
RenderBackend::RenderBackend(int w, int h)
    : width_(w), height_(h), front_buffer_(w, h) {
  init_console();
}

RenderBackend::~RenderBackend() { restore_console(); }

// ---------------------------------------------------------------------------
// Resize / invalidate
// ---------------------------------------------------------------------------
void RenderBackend::resize(int w, int h) {
  width_ = w;
  height_ = h;
  front_buffer_.resize(w, h);
  needs_full_redraw_ = true;
}

void RenderBackend::invalidate() { needs_full_redraw_ = true; }

// ---------------------------------------------------------------------------
// Move cursor below grid
// ---------------------------------------------------------------------------
void RenderBackend::move_cursor_below() {
  // Reset attributes, position at column 1 of the line just below the grid,
  // then show the cursor so the player sees it when typing.
  char buf[32];
  std::snprintf(buf, sizeof(buf), "\033[0m\033[%d;1H\033[?25h", height_ + 1);
  std::cout << buf << std::flush;
}

// ---------------------------------------------------------------------------
// emit_cell – output a single cell with minimal colour changes
// ---------------------------------------------------------------------------
void RenderBackend::emit_cell(std::string &out, const Cell &cell, int &cur_fg,
                              int &cur_bg) {
  // Transparent cells become a space (erase what was there before).
  // The compositor guarantees output planes are fully opaque, but this
  // fallback is safe for direct use of raw planes.
  char32_t glyph = cell.transparent ? U' ' : cell.glyph;

  // Only emit foreground colour if the glyph is visible (not a space)
  if (glyph != U' ' && cell.fg != cur_fg) {
    char buf[20];
    std::snprintf(buf, sizeof(buf), "\033[38;5;%dm", cell.fg);
    out += buf;
    cur_fg = cell.fg;
  }
  if (cell.bg != cur_bg) {
    char buf[20];
    std::snprintf(buf, sizeof(buf), "\033[48;5;%dm", cell.bg);
    out += buf;
    cur_bg = cell.bg;
  }
  to_utf8(glyph, out);
}

// ---------------------------------------------------------------------------
// present – diff-based frame output
// ---------------------------------------------------------------------------
void RenderBackend::present(const RenderPlane &frame) {
  std::string output;
  output.reserve(static_cast<size_t>(width_) * height_ * 8);

  // Always hide cursor while drawing (prevents flicker)
  output += "\033[?25l";

  int cur_fg = -1, cur_bg = -1;

  if (needs_full_redraw_) {
    // ---- Full redraw ----
    output += "\033[1;1H"; // top-left
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x)
        emit_cell(output, frame.get(x, y), cur_fg, cur_bg);
      output += "\033[0m\r\n"; // reset, CR+LF
      cur_fg = cur_bg = -1;
    }
    needs_full_redraw_ = false;
  } else {
    // ---- Incremental diff with run batching ----
    for (int y = 0; y < height_; ++y) {
      int x = 0;
      while (x < width_) {
        // skip unchanged cells
        while (x < width_ && frame.get(x, y) == front_buffer_.get(x, y))
          ++x;
        if (x >= width_)
          break;

        // found a dirty run – move cursor to its start
        char buf[20];
        std::snprintf(buf, sizeof(buf), "\033[%d;%dH", y + 1, x + 1);
        output += buf;

        // emit all consecutive dirty cells without further cursor moves
        while (x < width_ && frame.get(x, y) != front_buffer_.get(x, y)) {
          emit_cell(output, frame.get(x, y), cur_fg, cur_bg);
          ++x;
        }
      }
    }
    output += "\033[0m"; // final reset
  }

  std::cout << output << std::flush;

  // Update front buffer
  for (int y = 0; y < height_; ++y)
    for (int x = 0; x < width_; ++x)
      front_buffer_.get(x, y) = frame.get(x, y);
}