#include "render_backend.hpp"
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int RenderBackend::instance_count_ = 0;

static void hide_cursor() { std::cout << "\033[?25l"; }
static void show_cursor() { std::cout << "\033[?25h"; }

void RenderBackend::init_console() {
    if (instance_count_++ != 0) return;
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
#endif
    hide_cursor();
}

void RenderBackend::restore_console() {
    if (--instance_count_ != 0) return;
    show_cursor();
}

RenderBackend::RenderBackend(int w, int h)
    : width_(w), height_(h), front_buffer_(w, h) {
    init_console();
}

RenderBackend::~RenderBackend() {
    restore_console();
}

void RenderBackend::resize(int w, int h) {
    width_ = w;
    height_ = h;
    front_buffer_.resize(w, h);
    needs_full_redraw_ = true;
}

void RenderBackend::invalidate() {
    needs_full_redraw_ = true;
}

void RenderBackend::move_cursor_below() {
    std::cout << "\033[0m\033[" << (height_ + 1) << ";1H";
    show_cursor();
    std::cout << std::flush;
}

void RenderBackend::emit_cell(std::string& out, const Cell& cell,
                              int& cur_fg, int& cur_bg) {
    char glyph = (cell.glyph == '\0') ? ' ' : cell.glyph;

    if (cell.fg != cur_fg) {
        out.append("\033[38;5;").append(std::to_string(cell.fg)).append("m");
        cur_fg = cell.fg;
    }
    if (cell.bg != cur_bg) {
        out.append("\033[48;5;").append(std::to_string(cell.bg)).append("m");
        cur_bg = cell.bg;
    }
    out += glyph;
}

void RenderBackend::present(const RenderPlane& frame) {
    std::string output;
    output.reserve(static_cast<size_t>(width_) * height_ * 6);
    output += "\033[?25l"; // hide cursor during draw

    int cur_fg = -1, cur_bg = -1;

    if (needs_full_redraw_) {
        output += "\033[1;1H";
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x)
                emit_cell(output, frame.get(x, y), cur_fg, cur_bg);
            output += "\033[0m\r\n";   // reset attributes, go to column 1, next line
            cur_fg = cur_bg = -1;
        }
        needs_full_redraw_ = false;
    } else {
        for (int y = 0; y < height_; ++y) {
            int x = 0;
            while (x < width_) {
                // skip unchanged cells
                while (x < width_ && frame.get(x, y) == front_buffer_.get(x, y))
                    ++x;
                if (x >= width_) break;

                // found a dirty run – move cursor to its start
                output += "\033[" + std::to_string(y + 1) + ";" +
                          std::to_string(x + 1) + "H";
                cur_fg = cur_bg = -1;

                // emit all consecutive dirty cells in this run
                while (x < width_ && frame.get(x, y) != front_buffer_.get(x, y)) {
                    emit_cell(output, frame.get(x, y), cur_fg, cur_bg);
                    ++x;
                }
            }
        }
        output += "\033[0m";   // reset attributes after the diff
    }

    std::cout << output << std::flush;

    // update front buffer
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            front_buffer_.get(x, y) = frame.get(x, y);
}