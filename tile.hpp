#pragma once 

#include <string>

struct Tile {
	// Symbol of tile 
	char glyph;

	// can walk 
	bool is_walkable;
	bool transparent;
	bool is_explored;
	std::string name; 

	int fg_color;
	int bg_color;

	// Constructor 
	Tile() = default;
	// sym for glyph 
	Tile(char sym, std::string n, bool w, bool t)
		: glyph(sym) , name(n), is_walkable(w), is_transparent(t),
		is_explored(false), fg_color(7), bg_color(0) {}

	// utility 
	const is_wall() { return !is_walkable && !is_transparent }
	const is_floor() { return is_walkable && is_transparent}
};
	




