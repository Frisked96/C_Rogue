#pragma once

#include <iostream>
#include <vector>

using namespace std;

class Terminal_renderer {
private:
  int height;
  int width;
  vector<vector<char>> view_grid;

public:
  Terminal_renderer(int h, int w);
  void draw();
  void set_tile(int x, int y, char c);
};
