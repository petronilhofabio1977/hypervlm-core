#include "hvlm/QDNode96.hpp"
#pragma once
#include <string>
#include <vector>

namespace hvlm {

class HyperVLM_VM {
public:
    bool load(const std::string &file);
    void run();
    bool step();
};

}

// Executa um programa VLM usando o Dynamic Read Engine
void run_with_dre(std::vector<QDNode96>& nodes,
                  const std::vector<std::pair<int,int>>& subtree);

