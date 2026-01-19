#include "hvlm/VM.hpp"
#include "hvlm/QDNode96.hpp"
#include "hvlm/DRE.hpp"
#include <iostream>
#include <fstream>
#include <vector>

// ===============================
//   NAMESPACE hvlm — VM ORIGINAL
// ===============================

namespace hvlm {

bool HyperVLM_VM::load(const std::string &f) {
    std::cout << "[VM] loading " << f << std::endl;
    std::ifstream ifs(f, std::ios::binary);
    return ifs.good();
}

void HyperVLM_VM::run() {
    while(step());
}

bool HyperVLM_VM::step() {
    static int i = 0;
    if (i++ > 1) return false;
    std::cout << "[VM] step" << std::endl;
    return true;
}

} // namespace hvlm


// ===============================
//   FUNÇÃO GLOBAL — run_with_dre
// ===============================

void run_with_dre(std::vector<QDNode96>& nodes,
                  const std::vector<std::pair<int,int>>& subtree)
{
    std::cout << "\n[hvlm::DRE] run_with_dre started\n";
    std::cout << "[hvlm::DRE] nodes=" << nodes.size()
              << " subtrees=" << subtree.size() << "\n";

    hvlm::DRE dre(nodes, subtree);
    dre.init_root();

    size_t visited = 0;
    while (auto idx = dre.next_node()) {
        std::cout << "[hvlm::DRE] visiting node=" << *idx
                  << " morton=" << nodes[*idx].morton << "\n";
        visited++;
    }

    std::cout << "[hvlm::DRE] visited total = " << visited << "\n\n";
}

