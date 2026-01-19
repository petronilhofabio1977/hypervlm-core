#pragma once
#include "materialized_tile.hpp"
#include "blockid.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>

namespace hypervlm {

struct ShardIndexEntry {
    int64_t x;
    int64_t y;
    int64_t z;
    uint64_t offset;
    uint32_t size;
    uint32_t flags;
};

class DiskShard {
public:
    DiskShard(std::string path);
    ~DiskShard();

    bool open();
    std::optional<ShardIndexEntry> find_entry(int64_t x, int64_t y, int64_t z) const;
    std::optional<MaterializedTilePtr> load_tile(BlockId const& id) const;
    bool append_tile(MaterializedTilePtr tile);
    bool flush_index();
    std::string path() const { return path_; }

private:
    std::string path_;
    int fd_ = -1;
    void* mmap_base_ = nullptr;
    size_t mmap_size_ = 0;
    mutable std::mutex mtx_;

    std::vector<ShardIndexEntry> index_entries_;
    std::unordered_map<std::string, size_t> index_map_;

    bool load_index_from_file();
    bool remap_mmap(size_t new_size);
    static std::string key_for(int64_t x, int64_t y, int64_t z);
};

} // namespace hypervlm
