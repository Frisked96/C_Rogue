#pragma once 

#include <iostream>
#include <vector>
#include "tile.hpp"

/*Takes in int x and int y as size, vector of Tiles 
*/
class Game_map {
	private:
		int width; 
		int height; 
		std::vector<std::vector<Tile>> tiles;
	public:
		Game_map(int w, int h);
		void set_tile(int x, int y);
};
