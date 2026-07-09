#pragma once

#include "render_types.hpp"
#include <string>

// Double‑buffered ANSI terminal backend, tuned for Windows Terminal / ConPTY.
// Automatically enables UTF‑8 and virtual terminal processing, hides the
// cursor during frame draws, and restores it afterwards.
//
// Important: every cell is assumed to occupy exactly one terminal column.
// Only use codepoints that are "narrow" (ASCII, box‑drawing, Latin‑1, …).
// Wide characters (CJK, emoji) will break grid alignment.
class RenderBackend {
public:
    RenderBackend(int w, int h);
    ~RenderBackend();

    // Push a completed frame to the terminal. Diffs against the previous frame
    // and outputs only changed cells in contiguous runs.
    void present(const RenderPlane& frame);

    // Resize the internal front buffer. Next present() performs a full redraw.
    void resize(int w, int h);

    // Force the next present() to redraw every cell.
    void invalidate();

    // Move the cursor to the line below the grid, reset attributes, and show
    // the cursor so UI text can be printed safely.
    void move_cursor_below();

    int width()  const noexcept { return width_; }
    int height() const noexcept { return height_; }

private:
    int width_, height_;
    RenderPlane front_buffer_;     // mirrors what is currently on the terminal
    bool needs_full_redraw_ = true;

    static int instance_count_;

    static void init_console();
    static void restore_console();

    // Convert a codepoint to a UTF‑8 byte sequence.
    static std::string to_utf8(char32_t cp);

    // Append ANSI escape codes + glyph for a single cell.
    void emit_cell(std::string& out, const Cell& cell, int& cur_fg, int& cur_bg);
};