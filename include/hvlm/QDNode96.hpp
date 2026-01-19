#pragma once
#include <cstdint>

struct QDNode96 {
    uint64_t morton;           // 0x00

    uint32_t child_index;      // 0x08
    uint16_t class_id;         // 0x0C
    uint16_t namespace_id;     // 0x0E

    uint32_t arg0;             // 0x10
    uint32_t arg1;             // 0x14
    uint32_t arg2;             // 0x18
    uint32_t arg3;             // 0x1C

    uint32_t material_id;      // 0x20
    uint16_t flags;            // 0x24
    uint16_t prefetch_slot;    // 0x26

    uint16_t relevance_weight;     // 0x28
    uint16_t priority_hint;        // 0x2A
    uint16_t integral_relevance;   // 0x2C
    uint16_t integral_cost;        // 0x2E
    uint16_t integral_physics;     // 0x30
    uint16_t skip_hint;            // 0x32

    uint32_t physics_hint;     // 0x34
    uint32_t reserved0;        // 0x38

    uint16_t crc16;            // 0x3C
    uint8_t version;           // 0x3E
    uint8_t padding1;          // 0x3F

    uint64_t extra;            // 0x40
    uint32_t tag_hash;         // 0x48
    uint32_t reserved2;        // 0x4C

    uint8_t user_padding[16];  // 0x50
};
static_assert(sizeof(QDNode96) == 96, "QDNode96 must be 96 bytes");
