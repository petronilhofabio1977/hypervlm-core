#pragma once
#include <cstdint>

namespace hvlm {

// expand bits helper
inline uint64_t expand(uint32_t x) {
    uint64_t v = x & 0x1fffff;
    v = (v | (v << 32)) & 0x1f00000000ffff;
    v = (v | (v << 16)) & 0x1f0000ff0000ff;
    v = (v | (v << 8))  & 0x100f00f00f00f00f;
    v = (v | (v << 4))  & 0x10c30c30c30c30c3;
    v = (v | (v << 2))  & 0x1249249249249249;
    return v;
}

inline uint64_t morton3(uint32_t x, uint32_t y, uint32_t z) {
    return (expand(x) << 2) | (expand(y) << 1) | expand(z);
}

} // namespace hvlm
