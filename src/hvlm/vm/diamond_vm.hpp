#pragma once
#include <vector>
#include "../../../include/qdnode/qdnode.hpp"

class DiamondVM {
public:
    void load(const std::vector<QDNode96>& n){ nodes=n; }
    void run();
private:
    std::vector<QDNode96> nodes;
};
