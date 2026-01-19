#pragma once
#include "materialized_tile.hpp"
#include "blockid.hpp"
#include "diskshard.hpp"
#include <optional>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace hypervlm {

class DiskCache {
public:
    explicit DiskCache(std::string directory = "./shards");

    std::optional<MaterializedTilePtr> load(BlockId const& id);
    void store(MaterializedTilePtr tile);
    void clear();

private:
    std::string dir_;
    mutable std::mutex mtx_;
    std::unordered_map<int64_t, std::shared_ptr<DiskShard>> shards_;

    std::shared_ptr<DiskShard> shard_for_lod(int64_t lod);
    std::string shard_path(int64_t lod) const;
};

} // namespace hypervlm
