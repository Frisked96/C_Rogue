#pragma once

#include "render_types.hpp"
#include <string>

// Double-buffered ANSI terminal backend, optimised for Windows Terminal and
// ConPTY hosts (Windows 10 version 1511+). Automatically enables virtual
// terminal processing, hides the cursor, and restores it on destruction.
class RenderBackend {
public:
    RenderBackend(int w, int h);
    ~RenderBackend();

    // Push a completed frame to the terminal. Diffs against the previous frame
    // and outputs only changed cells, in contiguous runs to reduce escape
    // sequences.
    void present(const RenderPlane& frame);

    // Resize the internal front buffer. The next present() will perform a full
    // redraw. Call this after the terminal window size changes.
    void resize(int w, int h);

    // Force the next present() to redraw every cell (e.g. after a terminal
    // mode switch).
    void invalidate();

    // Move the cursor to the line below the rendered grid, reset attributes,
    // and show the cursor so UI text can be printed safely.
    void move_cursor_below();

    int width()  const noexcept { return width_; }
    int height() const noexcept { return height_; }

private:
    int width_;
    int height_;
    RenderPlane front_buffer_; // mirrors what is currently on the terminal
    bool needs_full_redraw_ = true;

    // One-time Windows console setup (VT mode, cursor hide).
    static void ensure_console_ready();
    static void restore_console();
    static int  s_instance_count; // to manage cursor show/hide

    // Append the ANSI codes + character for a single cell to `out`.
    // `cur_fg` / `cur_bg` are tracked so redundant colour changes are skipped.
    void emit_cell(std::string& out, const Cell& cell, int& cur_fg, int& cur_bg);
};