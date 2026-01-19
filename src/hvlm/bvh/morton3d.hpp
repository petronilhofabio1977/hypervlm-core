#pragma once
#include <cstdint>
static inline uint64_t split3(uint32_t x){
    uint64_t v = x & 0x1fffff;
    v = (v | v<<32)&0x1f00000000ffff;
    v = (v | v<<16)&0x1f0000ff0000ff;
    v = (v | v<<8 )&0x100f00f00f00f00f;
    v = (v | v<<4 )&0x10c30c30c30c30c3;
    v = (v | v<<2 )&0x1249249249249249;
    return v;
}
static inline uint64_t morton3D(uint32_t x,uint32_t y,uint32_t z){
    return (split3(x)<<2)|(split3(y)<<1)|split3(z);
}
