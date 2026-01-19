#pragma once
#include <vector>
#include <algorithm>
#include <cstdint>
#include "hvlm/HVNode.hpp"

namespace hvlm {

struct BVHNode {
    uint64_t morton;
    uint32_t left, right;
};

class LBVH {
public:
    static std::vector<BVHNode> build(const std::vector<HVNode96>& nodes);
};

}
