#pragma once
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

template <typename IDType = uint32_t> class SpatialGrid {
private:
  static constexpr int kChunkBits = 4;
  static constexpr int kChunkSize = 1 << kChunkBits;
  static constexpr int kChunkMask = kChunkSize - 1;
  static constexpr int kCellsPerChunk = kChunkSize * kChunkSize * kChunkSize;
  static constexpr uint32_t kNull = std::numeric_limits<uint32_t>::max();

  struct Node {
    IDType id;
    uint32_t next;
  };

  struct Chunk {
    std::vector<uint32_t> heads;
    Chunk() : heads(kCellsPerChunk, kNull) {}
  };

  std::unordered_map<uint64_t, Chunk> chunks_;
  std::vector<Node> pool_;
  uint32_t free_head_ = kNull;

  static constexpr uint64_t make_chunk_key(int cx, int cy, int cz) noexcept {
    return ((uint64_t)(uint32_t)cx & 0x1FFFFF) |
           (((uint64_t)(uint32_t)cy & 0x1FFFFF) << 21) |
           (((uint64_t)(uint32_t)cz & 0x1FFFFF) << 42);
  }

  static constexpr int chunk_coord(int v) noexcept { return v >> kChunkBits; }
  static constexpr int local_coord(int v) noexcept { return v & kChunkMask; }

  static constexpr uint32_t local_index(int x, int y, int z) noexcept {
    int lx = local_coord(x), ly = local_coord(y), lz = local_coord(z);
    return (uint32_t)(lx + ly * kChunkSize + lz * kChunkSize * kChunkSize);
  }

  uint32_t alloc_node(IDType id, uint32_t next) {
    if (free_head_ != kNull) {
      uint32_t idx = free_head_;
      free_head_ = pool_[idx].next;
      pool_[idx].id = id;
      pool_[idx].next = next;
      return idx;
    }
    pool_.push_back(Node{id, next});
    return (uint32_t)(pool_.size() - 1);
  }

  void free_node(uint32_t idx) noexcept {
    pool_[idx].next = free_head_;
    free_head_ = idx;
  }

  Chunk *find_chunk(int x, int y, int z) noexcept {
    auto it = chunks_.find(
        make_chunk_key(chunk_coord(x), chunk_coord(y), chunk_coord(z)));
    return it != chunks_.end() ? &it->second : nullptr;
  }

  const Chunk *find_chunk(int x, int y, int z) const noexcept {
    auto it = chunks_.find(
        make_chunk_key(chunk_coord(x), chunk_coord(y), chunk_coord(z)));
    return it != chunks_.end() ? &it->second : nullptr;
  }

public:
  void add(IDType id, int x, int y, int z) {
    Chunk &c =
        chunks_[make_chunk_key(chunk_coord(x), chunk_coord(y), chunk_coord(z))];
    uint32_t local = local_index(x, y, z);
    c.heads[local] = alloc_node(id, c.heads[local]);
  }

  void remove(IDType id, int x, int y, int z) {
    Chunk *c = find_chunk(x, y, z);
    if (!c)
      return;
    uint32_t local = local_index(x, y, z);
    uint32_t cur = c->heads[local];
    uint32_t prev = kNull;
    while (cur != kNull) {
      if (pool_[cur].id == id) {
        uint32_t next = pool_[cur].next;
        if (prev == kNull)
          c->heads[local] = next;
        else
          pool_[prev].next = next;
        free_node(cur);
        return;
      }
      prev = cur;
      cur = pool_[cur].next;
    }
  }

  void move(IDType id, int ox, int oy, int oz, int nx, int ny, int nz) {
    uint64_t okey =
        make_chunk_key(chunk_coord(ox), chunk_coord(oy), chunk_coord(oz));
    uint64_t nkey =
        make_chunk_key(chunk_coord(nx), chunk_coord(ny), chunk_coord(nz));
    uint32_t olocal = local_index(ox, oy, oz);
    uint32_t nlocal = local_index(nx, ny, nz);

    if (okey == nkey && olocal == nlocal)
      return;

    auto oit = chunks_.find(okey);
    if (oit != chunks_.end()) {
      Chunk &oc = oit->second;
      uint32_t cur = oc.heads[olocal];
      uint32_t prev = kNull;
      while (cur != kNull) {
        if (pool_[cur].id == id) {
          uint32_t next = pool_[cur].next;
          if (prev == kNull)
            oc.heads[olocal] = next;
          else
            pool_[prev].next = next;

          Chunk &nc = chunks_[nkey];
          pool_[cur].next = nc.heads[nlocal];
          nc.heads[nlocal] = cur;
          return;
        }
        prev = cur;
        cur = pool_[cur].next;
      }
    }
    Chunk &nc = chunks_[nkey];
    nc.heads[nlocal] = alloc_node(id, nc.heads[nlocal]);
  }

  std::vector<IDType> get_at(int x, int y, int z) const {
    std::vector<IDType> out;
    const Chunk *c = find_chunk(x, y, z);
    if (c) {
      uint32_t cur = c->heads[local_index(x, y, z)];
      while (cur != kNull) {
        out.push_back(pool_[cur].id);
        cur = pool_[cur].next;
      }
    }
    return out;
  }

  bool has_any(int x, int y, int z) const noexcept {
    const Chunk *c = find_chunk(x, y, z);
    return c && c->heads[local_index(x, y, z)] != kNull;
  }

  void clear() noexcept {
    chunks_.clear();
    pool_.clear();
    free_head_ = kNull;
  }
};