#pragma once
// This header provides thin wrappers to call your ATMBridge / DiamondMemory / ILME.
// It compiles even if ATMBridge is not present. If your atm_bridge/include/ATMBridge.hpp
// is available and you want to use it, include the header path in CMake and this file
// will forward tasks. Otherwise default to std::thread pool loops.

#ifdef USE_ATM_BRIDGE
#include "../../../atm_bridge/include/ATMBridge.hpp"
#endif

#include <functional>
#include <vector>
#include <thread>
#include <atomic>

struct TaskQueue {
#ifdef USE_ATM_BRIDGE
    ATMBridge bridge;
    TaskQueue(int n=0):bridge(n){}
    template<typename F> void submit(F&&f){ bridge.submit(std::function<void()>(f)); }
    void shutdown(){ bridge.shutdown(); }
#else
    std::atomic<int> next{0};
    std::vector<std::thread> threads;
    int nthreads=0;
    TaskQueue(int n=0){ nthreads = n>0 ? n : std::max(1u, std::thread::hardware_concurrency()? std::thread::hardware_concurrency() : 4u); for(int i=0;i<nthreads;i++) threads.emplace_back([&](){/* idle - work is pulled by caller */}); }
    template<typename F> void submit(F&&f){ f(); } // synchronous fallback
    void shutdown(){ for(auto &t: threads){ if(t.joinable()) t.join(); } }
#endif
};
