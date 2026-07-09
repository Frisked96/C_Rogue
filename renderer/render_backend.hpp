#pragma once

#include "render_types.hpp"
#include <string>

class RenderBackend {
public:
    RenderBackend(int w, int h);
    ~RenderBackend();

    void present(const RenderPlane& frame);
    void resize(int w, int h);
    void invalidate();
    void move_cursor_below();

    int width()  const noexcept { return width_; }
    int height() const noexcept { return height_; }

private:
    int width_, height_;
    RenderPlane front_buffer_;
    bool needs_full_redraw_ = true;

    static int instance_count_;

    static void init_console();
    static void restore_console();

    void emit_cell(std::string& out, const Cell& cell, int& cur_fg, int& cur_bg);
};