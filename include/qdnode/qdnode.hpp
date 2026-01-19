#pragma once
#include <cstdint>
#include <cstring>
#include <cassert>

static inline uint16_t crc16_compute(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)p[i] << 8;
        for (int k = 0; k < 8; k++)
            crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
    }
    return crc;
}

#pragma pack(push,1)
struct QDNode96 {

    // 1) Campos principais (64 bits cada)
    uint64_t morton;        // 0–7
    uint64_t child_index;   // 8–15

    // 2) Args (6 × 4 bytes = 24 bytes)
    uint32_t arg0;          // 16–19
    uint32_t arg1;          // 20–23
    uint32_t arg2;          // 24–27
    uint32_t arg3;          // 28–31
    uint32_t material_id;   // 32–35
    uint32_t flags32;       // 36–39  (substitui vários pequenos)

    // 3) Campos auxiliares (8 bytes cada)
    uint64_t physics_hint;  // 40–47
    uint64_t reserved0;     // 48–55

    // 4) Controle e versão (2 + 1 + padding)
    uint16_t crc16;         // 56–57
    uint8_t version;        // 58
    uint8_t pad[37];        // 59–95 (para completar 96 bytes)

    QDNode96() {
        memset(this, 0, sizeof(QDNode96));
        version = 1;
    }

    void finalize(){
        crc16 = 0;
        crc16 = crc16_compute(this, sizeof(QDNode96));
    }
};
#pragma pack(pop)

static_assert(sizeof(QDNode96) == 96, "QDNode96 must be EXACTLY 96 bytes");
