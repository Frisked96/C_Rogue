#include <iostream>
#include <vector>

using namespace std;

class Terminal_render {
private:
  int height;
  int width;
  vector<vector<char>> grid;

public:
  Terminal_render(int w, int h)
      : height(h), width(w), grid(h, vector<char>(w, '.')) {}
  void draw() {
    for (auto &row : grid) {
      for (char c : row)
        cout << c;
      cout << "\n";
    }
  }
  void set_tile(int y, int x, char c) { 
	  if(y>=0 && y<height && x>=0 && x<width){
		  grid[y][x] = c;
	  }
  }
};

int main() {
  Terminal_render render(10, 10);
  render.set_tile(4, 4, '@');
  render.draw();
  return 0;
}
