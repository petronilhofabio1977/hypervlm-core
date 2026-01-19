#include "atm.hpp"
#include <chrono>
#include <iostream>
#include <cmath>
#include <atomic>

using clk = std::chrono::steady_clock;

static inline void microkernel_fma(int iters, double *s) {
    double a=s[0], b=s[1], c=s[2];
    for (int i=0;i<iters;++i) {
        a = std::fma(a,b,c);
        b = std::fma(b,c,a);
        c = std::fma(c,a,b);
    }
    s[0]=a; s[1]=b; s[2]=c;
}

int main() {
    uint64_t n_tasks = 1000000;
    int k = 100;
    uint64_t n_virtual_nodes = 4096;

    atm::SchedulerConfig cfg;
    cfg.n_virtual_nodes = n_virtual_nodes;
    atm::Scheduler sched(cfg);

    std::atomic<uint64_t> done{0};

    auto t0 = clk::now();
    for (uint64_t i=0;i<n_tasks;++i) {
        uint64_t vnode = i % n_virtual_nodes;
        sched.submit([&done,k](){
            double s[3] = {1.23,2.34,3.45};
            microkernel_fma(k, s);
            done++;
        }, vnode);
    }
    sched.wait_all();
    auto t1 = clk::now();

    double dt = std::chrono::duration<double>(t1 - t0).count();
    uint64_t ops = n_tasks * uint64_t(k) * 3;

    std::cout << "ATM Benchmark Math\n";
    std::cout << "Time: " << dt << " s\n";
    std::cout << "Ops/s: " << ops / dt << "\n";
}
