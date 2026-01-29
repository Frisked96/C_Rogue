#pragma once 

#include <iostream>
#include <vector>
#include "tile.hpp"

class Game_map {
	private:
		int width; 
		int height; 
		vector<vector<Tile>> map;
	public:
		Game_map(int x, int y);
		void set_tile(int x, int y);
};
