#pragma once
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "entity.hpp"

class Game_map;

class SpatialGrid {
private:
    std::unordered_map<uint64_t, std::vector<EntityID>> grid;

    uint64_t get_key(int x, int y, int z) const {
        return ((uint64_t)(uint32_t)x) | (((uint64_t)(uint32_t)y) << 20) | (((uint64_t)(uint32_t)z) << 40);
    }

public:
    void add(EntityID id, int x, int y, int z) {
        grid[get_key(x, y, z)].push_back(id);
    }

    void remove(EntityID id, int x, int y, int z) {
        auto& vec = grid[get_key(x, y, z)];
        vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
    }

    void move(EntityID id, int ox, int oy, int oz, int nx, int ny, int nz) {
        remove(id, ox, oy, oz);
        add(id, nx, ny, nz);
    }

    const std::vector<EntityID>& get_at(int x, int y, int z) const {
        static const std::vector<EntityID> empty;
        auto it = grid.find(get_key(x, y, z));
        return (it != grid.end()) ? it->second : empty;
    }

    bool is_blocked(int x, int y, int z, const Game_map& map) const;
};
