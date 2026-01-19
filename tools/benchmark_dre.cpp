#include <iostream>
#include <vector>
#include <chrono>
#include "hvlm/QDNode96.hpp"
#include "hvlm/bvh/LBVH_helpers.hpp"
#include "hvlm/DRE.hpp"

using namespace std::chrono;
using namespace std;

int main(int argc,char** argv){
    int N = 100000;
    if (argc > 1) N = atoi(argv[1]);

    cout << "[bench] Creating " << N << " QD nodes via LBVH_helpers\n";
    auto nodes = hvlm::LBVH_helpers::make_n_qd(N);
    auto subtree = hvlm::LBVH_helpers::make_n_subtrees(N);

    // sequential walk (simple loop)
    auto seq_nodes = nodes;
    auto t0 = high_resolution_clock::now();
    volatile uint64_t acc = 0;
    for (size_t i=0;i<seq_nodes.size();++i) {
        acc += seq_nodes[i].morton;
    }
    auto t1 = high_resolution_clock::now();

    // DRE walk
    auto dre_nodes = nodes;
    auto t2 = high_resolution_clock::now();
    hvlm::DRE dre(dre_nodes, subtree, hvlm::DREConfig{});
    dre.init_root();
    size_t visited = 0;
    while (auto idx = dre.next_node()) {
        acc += dre_nodes[*idx].morton;
        visited++;
    }
    auto t3 = high_resolution_clock::now();

    auto seq_ms = duration_cast<milliseconds>(t1-t0).count();
    auto dre_ms = duration_cast<milliseconds>(t3-t2).count();

    cout << "[bench] seq_ms=" << seq_ms << " dre_ms=" << dre_ms
         << " visited=" << visited << " checksum=" << acc << "\n";
    return 0;
}
