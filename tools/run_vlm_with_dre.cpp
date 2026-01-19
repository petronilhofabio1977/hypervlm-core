#include "hvlm/VM.hpp"
#include "hvlm/QDNode96.hpp"
#include <fstream>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) { printf("Usage: run_dre file.vlm\n"); return 1; }

    auto nodes = load_vlm(argv[1]);   // Função já existente no projeto
    auto subtree = compute_subtrees(nodes); // existente no LBVH/diamond pipeline

    VM vm;
    vm.run_with_dre(nodes, subtree);
}
