#include "LBVH.hpp"

namespace hvlm {

std::vector<BVHNode> LBVH::build(const std::vector<HVNode96>& input) {
    std::vector<BVHNode> out;
    out.reserve(input.size());

    // sort by morton
    std::vector<std::pair<uint64_t, size_t>> keys;
    keys.reserve(input.size());

    for (size_t i=0;i<input.size();i++)
        keys.push_back({input[i].morton, i});

    std::sort(keys.begin(), keys.end(),
              [](auto&a, auto&b){ return a.first < b.first; });

    // create leaf nodes
    for (auto& k : keys) {
        BVHNode n;
        n.morton = k.first;
        n.left   = n.right = 0xFFFFFFFF;
        out.push_back(n);
    }

    // INTERNAL NODES WILL BE ADDED LATER (STEP 2)
    return out;
}

} // namespace hvlm
