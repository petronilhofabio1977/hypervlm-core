#pragma once
#include <cstdint>
#include <cstring>

namespace hvlm {

struct HVNode96 {
    uint64_t morton;
    uint64_t child_index;

    uint32_t arg0, arg1, arg2, arg3;
    uint32_t arg4, arg5, material_id;

    uint16_t flags, prefetch_slot;
    uint16_t relevance, priority;

    uint64_t physics_hint;
    uint64_t reserved0;

    uint16_t crc16;
    uint8_t version;
    uint8_t padding[25];

    HVNode96() {
        memset(this, 0, sizeof(HVNode96));
        version = 1;
    }

    uint16_t compute_crc16() const {
        const uint8_t* d = reinterpret_cast<const uint8_t*>(this);
        uint32_t s = 0;
        for (size_t i=0; i<sizeof(HVNode96)-2; i++) s+=d[i];
        return (uint16_t)(s & 0xFFFF);
    }
    void finalize() { crc16 = compute_crc16(); }
};

} // namespace hvlm
