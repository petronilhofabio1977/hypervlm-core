#pragma once
#include "blockid.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <cstring>

#ifdef __linux__
#include <sys/mman.h>
#endif

namespace hypervlm {

struct MMapShardView {
    void* base_ptr = nullptr;
    size_t map_size = 0;
    size_t data_offset = 0;
    size_t data_size = 0;
    const unsigned char* data_ptr() const noexcept {
        return reinterpret_cast<const unsigned char*>(
            static_cast<const unsigned char*>(base_ptr) + data_offset);
    }
};

struct MaterializedTile {
    BlockId id;
    std::vector<unsigned char> data;
    std::shared_ptr<MMapShardView> shard_view;
    mutable std::mutex mtx;

    MaterializedTile() = default;
    MaterializedTile(BlockId id_) : id(id_) {}

    const unsigned char* bytes() const noexcept {
        if (shard_view && shard_view->base_ptr) return shard_view->data_ptr();
        if (!data.empty()) return data.data();
        return nullptr;
    }
    size_t size() const noexcept {
        if (shard_view && shard_view->data_size) return shard_view->data_size;
        return data.size();
    }

    void ensure_owned() {
        std::lock_guard lk(mtx);
        if (shard_view && shard_view->base_ptr && shard_view->data_size) {
            data.resize(shard_view->data_size);
            std::memcpy(data.data(), shard_view->data_ptr(), shard_view->data_size);
            shard_view.reset();
        }
    }
};

using MaterializedTilePtr = std::shared_ptr<MaterializedTile>;

} // namespace hypervlm
