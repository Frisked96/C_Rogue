#pragma once 
#include "component.hpp"
#include <string>

// basic components for most entities 
class PositionComponent : public BaseComponent<PositionComponent> {
    public:
    int x;
    int y;

    PositionComponent(int x = 0, int y = 0) : x(x), y(y) {}
};

class RenderComponent : public BaseComponent<RenderComponent> {
    public:
    char glyph;

    RenderComponent(char glyph = '?') : glyph(glyph) {}
};

class NameComponent : public BaseComponent<NameComponent> {
    public:
    std::string name;
    
    NameComponent( const std::string& name = "") : name(name) {}
};

class BlockingComponent : public BaseComponent<BlockingComponent> {
    // No data as this is just a flag 
    // entities that have this block movement
};



























