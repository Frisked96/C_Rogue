#include "render_backend.hpp"
#include <iostream>
#include <algorithm>   // std::min
#include <cstring>     // std::memset (unused, removed)

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int RenderBackend::s_instance_count = 0;

// ---------------------------------------------------------------------------
// Windows console initialisation / teardown
// ---------------------------------------------------------------------------
void RenderBackend::ensure_console_ready() {
    if (s_instance_count++ != 0) return; // already set up

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    // Enable virtual terminal processing (required for ANSI escape sequences)
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    // Hide the cursor during frame updates
    std::cout << "\033[?25l";
#endif
}

void RenderBackend::restore_console() {
    if (--s_instance_count != 0) return; // still other instances alive

#ifdef _WIN32
    // Show cursor again
    std::cout << "\033[?25h" << std::flush;
#endif
}

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
RenderBackend::RenderBackend(int w, int h)
    : width_(w), height_(h), front_buffer_(w, h) {
    ensure_console_ready();
}

RenderBackend::~RenderBackend() {
    restore_console();
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------
void RenderBackend::resize(int w, int h) {
    width_  = w;
    height_ = h;
    front_buffer_.resize(w, h);
    needs_full_redraw_ = true;
}

// ---------------------------------------------------------------------------
// Invalidate
// ---------------------------------------------------------------------------
void RenderBackend::invalidate() {
    needs_full_redraw_ = true;
}

// ---------------------------------------------------------------------------
// Move cursor below grid
// ---------------------------------------------------------------------------
void RenderBackend::move_cursor_below() {
    // Show cursor, reset attributes, then move to column 1 of the line below
    std::cout << "\033[?25h\033[0m\033[" << (height_ + 1) << ";1H" << std::flush;
}

// ---------------------------------------------------------------------------
// emit_cell – output a single cell with minimal colour changes
// ---------------------------------------------------------------------------
void RenderBackend::emit_cell(std::string& out, const Cell& cell,
                              int& cur_fg, int& cur_bg) {
    char glyph = cell.glyph;
    if (glyph == '\0') glyph = ' '; // transparent → space

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

// ---------------------------------------------------------------------------
// present – diff‑based frame output
// ---------------------------------------------------------------------------
void RenderBackend::present(const RenderPlane& frame) {
    std::string output;
    output.reserve(static_cast<size_t>(width_) * height_ * 4);

    int cur_fg = -1;
    int cur_bg = -1;

    if (needs_full_redraw_) {
        // ---- Full redraw ----
        output += "\033[1;1H"; // cursor to top‑left
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                emit_cell(output, frame.get(x, y), cur_fg, cur_bg);
            }
            // Reset colours at end of row to avoid bleeding into next row
            output += "\033[0m\n";
            cur_fg = cur_bg = -1;
        }
        needs_full_redraw_ = false;
    } else {
        // ---- Incremental diff with run batching ----
        for (int y = 0; y < height_; ++y) {
            int x = 0;
            while (x < width_) {
                // Skip clean cells
                while (x < width_ &&
                       frame.get(x, y) == front_buffer_.get(x, y))
                    ++x;
                if (x >= width_) break;

                // Start of a dirty run – place cursor at the beginning of the run
                int run_start = x;
                output += "\033[" + std::to_string(y + 1) + ";" +
                          std::to_string(x + 1) + "H";
                cur_fg = cur_bg = -1; // force colour emission for first cell

                // Emit all consecutive dirty cells without moving cursor
                while (x < width_ &&
                       frame.get(x, y) != front_buffer_.get(x, y)) {
                    emit_cell(output, frame.get(x, y), cur_fg, cur_bg);
                    ++x;
                }
            }
        }
    }

    // Append a final reset so terminal state is clean after the frame
    if (!output.empty()) {
        output += "\033[0m";
        std::cout << output << std::flush;
    }

    // Update front buffer
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            front_buffer_.get(x, y) = frame.get(x, y);
}