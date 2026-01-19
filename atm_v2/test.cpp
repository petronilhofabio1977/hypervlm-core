#include "include/atm_v2.hpp"
#include <iostream>
#include <atomic>

int main() {
    atm2::ATMConfig cfg;
    cfg.virtual_nodes = 1'000'000;   // 1 milhão de nodos virtuais
    cfg.clusters = 256;              // clusters para distribuir nodos
    cfg.workers = 0;                 // auto: usar todos os núcleos

    atm2::ATMv2 atm(cfg);

    std::atomic<int> sum{0};

    // 100k tarefas
    for (int i = 0; i < 100000; ++i) {
        atm.submit([&sum]() {
            sum.fetch_add(1, std::memory_order_relaxed);
        }, i);
    }

    atm.wait_all();

    std::cout << "Sum = " << sum.load() << "\n";
    return 0;
}
