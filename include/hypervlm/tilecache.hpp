#pragma once
#include "materialized_tile.hpp"
#include "blockid.hpp"
#include <unordered_map>
#include <list>
#include <shared_mutex>

namespace hypervlm {

class TileCache {
public:
    explicit TileCache(size_t capacity_bytes = 128ull * 1024 * 1024);

    MaterializedTilePtr get(BlockId const& id);
    void put(MaterializedTilePtr tile);
    bool contains(BlockId const& id) const;
    void clear();

private:
    mutable std::shared_mutex mtx;
    size_t capacity_bytes;
    size_t used_bytes = 0;

    std::list<BlockId> lru_list;
    struct Entry { MaterializedTilePtr tile; size_t size; std::list<BlockId>::iterator it; };
    std::unordered_map<size_t, Entry> map;

    static size_t hash_id(BlockId const& id) noexcept;
    void evict_if_needed();
};

} // namespace hypervlm
