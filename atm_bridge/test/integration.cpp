#include "ATMBridge.hpp"
#include <iostream>
#include <atomic>
#include <thread>

using namespace std;
using namespace atmbridge;

int main() {
    BridgeConfig bc;
    bc.atm_cfg.virtual_nodes = 4096;
    bc.atm_cfg.clusters = 64;
    bc.atm_cfg.workers = 0; // auto
    bc.atm_cfg.enable_workstealing = true;
    bc.atm_cfg.enable_batching = true;
    bc.atm_cfg.verbose = false;
    bc.verbose = true;

    ATMBridge &bridge = ATMBridge::instance();
    bridge.init(bc);

    // Simulate APEX submitting 1000 tasks
    atomic<int> apex_count{0};
    for (int i=0;i<1000;++i) {
        bridge.submit_apex(i, [&apex_count,i](){
            // pretend work (fast)
            apex_count.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Simulate transformer submitting 2000 tasks
    atomic<int> trans_count{0};
    for (int i=0;i<2000;++i) {
        bridge.submit_transformer(10000 + i, [&trans_count](){
            trans_count.fetch_add(1, std::memory_order_relaxed);
        });
    }

    bridge.wait_all();
    cout << "APEX tasks done: " << apex_count.load() << "\\n";
    cout << "Transformer tasks done: " << trans_count.load() << "\\n";

    bridge.shutdown();
    return 0;
}
