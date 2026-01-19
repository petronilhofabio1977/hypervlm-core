#pragma once
#include <cstdint>
#include <tuple>
#include <string>

namespace hypervlm {

struct BlockId {
    int64_t x = 0;
    int64_t y = 0;
    int64_t z = 0;
    int64_t lod = 0;

    bool operator==(BlockId const& o) const noexcept {
        return std::tie(x,y,z,lod) == std::tie(o.x,o.y,o.z,o.lod);
    }
    bool operator!=(BlockId const& o) const noexcept { return !(*this==o); }

    std::string key() const {
        return std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z);
    }
};

} // namespace hypervlm
