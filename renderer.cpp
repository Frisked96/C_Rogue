#include "renderer.hpp"
#include <iostream>
#include <vector>

using namespace std;

Terminal_renderer::Terminal_renderer(int w, int h)
    : height(h), width(w), view_grid(h, vector<char>(w, '.')) {}

void Terminal_renderer::draw() {
  for (auto &row : view_grid) {
    for (char c : row)
      cout << c;
    cout << "\n";
  }
}

void Terminal_renderer::set_tile(int x, int y, char c) {
  if (y >= 0 && y < height && x >= 0 && x < width) {
    view_grid[y][x] = c;
  }
}
