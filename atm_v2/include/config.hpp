#pragma once
#include <cstdint>
#include <cstddef>

namespace atm2 {

struct ATMConfig {
    uint64_t virtual_nodes = 4096;  // número de nodos virtuais
    uint64_t clusters = 64;         // grupos de nodos para melhor distribuição
    uint64_t workers = 0;           // 0 → usar hardware_concurrency()

    uint64_t vram_gb = 32;          // VRAM simulada
    uint64_t page_size_kb = 64;     // tamanho de página da VRAM

    bool enable_workstealing = true;
    bool enable_batching = true;
    bool verbose = false;
};

} // namespace atm2
