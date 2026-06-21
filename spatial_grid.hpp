#pragma once
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Generic spatial hash map for a 3D grid world.
// Maps (x,y,z) positions to lists of IDs. IDType must be an integer type.
// Shared by Entity, Object, and any future system that needs spatial lookups.
template <typename IDType = uint32_t> class SpatialGrid {
private:
  std::unordered_map<uint64_t, std::vector<IDType>> grid_;

  static uint64_t make_key(int x, int y, int z) {
    return ((uint64_t)(uint32_t)x) | (((uint64_t)(uint32_t)y) << 20) |
           (((uint64_t)(uint32_t)z) << 40);
  }

public:
  void add(IDType id, int x, int y, int z) {
    grid_[make_key(x, y, z)].push_back(id);
  }

  void remove(IDType id, int x, int y, int z) {
    auto it = grid_.find(make_key(x, y, z));
    if (it != grid_.end()) {
      auto &vec = it->second;
      vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
      if (vec.empty())
        grid_.erase(it);
    }
  }

  void move(IDType id, int ox, int oy, int oz, int nx, int ny, int nz) {
    remove(id, ox, oy, oz);
    add(id, nx, ny, nz);
  }

  const std::vector<IDType> &get_at(int x, int y, int z) const {
    static const std::vector<IDType> empty;
    auto it = grid_.find(make_key(x, y, z));
    return (it != grid_.end()) ? it->second : empty;
  }

  bool has_any(int x, int y, int z) const {
    auto it = grid_.find(make_key(x, y, z));
    return it != grid_.end() && !it->second.empty();
  }

  void clear() { grid_.clear(); }
};
