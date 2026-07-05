#pragma once
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Generic spatial hash map for a 3D grid world.
// Maps (x,y,z) positions to lists of IDs. IDType must be an integer type.
template <typename IDType = uint32_t> class SpatialGrid {
private:
  std::unordered_map<uint64_t, std::vector<IDType>> grid_;

  // Safely packs 3D coordinates into a 64-bit key.
  static constexpr uint64_t make_key(int x, int y, int z) noexcept {
    return ((uint64_t)(uint32_t)x & 0x1FFFFF) |
           (((uint64_t)(uint32_t)y & 0x1FFFFF) << 21) |
           (((uint64_t)(uint32_t)z & 0x1FFFFF) << 42);
  }

public:
  void add(IDType id, int x, int y, int z) {
    grid_[make_key(x, y, z)].push_back(id);
  }

  void remove(IDType id, int x, int y, int z) {
    auto it = grid_.find(make_key(x, y, z));
    if (it != grid_.end()) {
      auto &vec = it->second;

      // O(1) removal via swap-and-pop (preserves order? No, but fast).
      auto item_it = std::find(vec.begin(), vec.end(), id);
      if (item_it != vec.end()) {
        *item_it = std::move(vec.back());
        vec.pop_back();
      }

      if (vec.empty()) {
        grid_.erase(it);
      }
    }
  }

  void move(IDType id, int ox, int oy, int oz, int nx, int ny, int nz) {
    remove(id, ox, oy, oz);
    add(id, nx, ny, nz);
  }

  const std::vector<IDType> &get_at(int x, int y, int z) const noexcept {
    static const std::vector<IDType> empty;
    auto it = grid_.find(make_key(x, y, z));
    return (it != grid_.end()) ? it->second : empty;
  }

  bool has_any(int x, int y, int z) const noexcept {
    auto it = grid_.find(make_key(x, y, z));
    return it != grid_.end() && !it->second.empty();
  }

  void clear() noexcept { grid_.clear(); }
};