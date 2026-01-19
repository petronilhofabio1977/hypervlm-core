#pragma once
#include "generators.hpp"
#include "tilecache.hpp"
#include "diskcache.hpp"
#include "threadpool.hpp"
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace hypervlm {

struct GenEntry {
    std::string prefix;
    GenFn fn;
};

class Core {
public:
    Core(size_t mem_limit_bytes = 128ull * 1024 * 1024,
         size_t threads = 4);
    ~Core();

    void register_generator(const std::string& prefix, GenFn fn);

    MaterializedTilePtr request_tile(BlockId id);
    void request_tile_async(BlockId id, std::function<void(MaterializedTilePtr)> cb);
    std::future<MaterializedTilePtr> request_tile_future(BlockId id);

    TileCache& cache() noexcept { return tile_cache; }
    DiskCache& disk_cache() noexcept { return disk_cache_; }

private:
    TileCache tile_cache;
    DiskCache disk_cache_;
    ThreadPool pool;

    std::vector<GenEntry> generators;
    std::mutex gen_mtx;

    MaterializedTilePtr generate_and_cache(BlockId const& id);
    std::optional<GenFn> find_generator_for(BlockId const& id);
};

} // namespace hypervlm
