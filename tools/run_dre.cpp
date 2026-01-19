#include <iostream>
#include <string>
#include "hvlm/QDNode96.hpp"
#include "hvlm/bvh/LBVH_helpers.hpp"

// declare the run_with_dre global (implemented in runtime wrapper)
void run_with_dre(std::vector<QDNode96>& nodes,
                  const std::vector<std::pair<int,int>>& subtree);

int main(int argc,char** argv){
    std::string infile;
    if (argc < 2) {
        std::cout << "[run_dre] no input, using dummy 5 nodes\n";
        auto qnodes = hvlm::LBVH_helpers::make_n_qd(5);
        auto subtree = hvlm::LBVH_helpers::make_n_subtrees(5);
        run_with_dre(qnodes, subtree);
        return 0;
    }
    infile = argv[1];
    // If file ends with .hv try to invoke hvlmc to generate .vlm then fallback
    bool built = false;
    if (infile.size() > 3 && infile.substr(infile.size()-3) == "hv") {
        if (system((std::string("./hvlmc ") + infile).c_str()) == 0) {
            std::string vlm = infile + ".vlm";
            // try to load using LBVH_helpers loader if exists
            // fallback: create test nodes
            (void)vlm;
            built = true;
        }
    }
    if (!built) {
        // fallback to N=100 nodes
        int N = 100;
        auto qnodes = hvlm::LBVH_helpers::make_n_qd(N);
        auto subtree = hvlm::LBVH_helpers::make_n_subtrees(N);
        run_with_dre(qnodes, subtree);
    } else {
        auto qnodes = hvlm::LBVH_helpers::make_n_qd(100);
        auto subtree = hvlm::LBVH_helpers::make_n_subtrees(100);
        run_with_dre(qnodes, subtree);
    }
    return 0;
}
