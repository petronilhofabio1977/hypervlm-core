#pragma once
#include <vector>
#include <utility>
#include <cstdint>
#include <cmath>
#include <algorithm>

#include "hvlm/QDNode96.hpp"

// Best-effort helpers for converting HVNodes -> QDNode96 and computing integrals.
// This header is intentionally conservative: doesn't reference HVNode if it doesn't exist.
// If HVNode type exists, the conversion function will be enabled via ADL / overload.

namespace hvlm {
namespace LBVH_helpers {

// Minimal QDNode generator (fallback)
inline std::vector<QDNode96> make_n_qd(size_t n) {
    std::vector<QDNode96> out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        QDNode96 q{};
        q.morton = (uint64_t)i;
        q.integral_relevance = 1;
        q.integral_cost = 1;
        q.integral_physics = 1;
        q.relevance_weight = 1;
        q.priority_hint = 1;
        q.skip_hint = 100;
        out.push_back(q);
    }
    return out;
}

// Trivial subtree ranges for N nodes
inline std::vector<std::pair<int,int>> make_n_subtrees(size_t n) {
    std::vector<std::pair<int,int>> out;
    out.reserve(n);
    for (int i = 0; i < (int)n; ++i) out.emplace_back(i, i);
    return out;
}

// Compute simple integrals from heuristic inputs (normalize & quantize into uint16)
inline uint16_t quant16(double v) {
    v = std::max(0.0, std::min(v, 65535.0));
    return (uint16_t)std::floor(v + 0.5);
}

struct IntegralInputs {
    double rel = 1.0;
    double cost = 1.0;
    double phys = 1.0;
};

// Default integrals function: assign simple weighted combination
inline void compute_integrals(QDNode96 &q, const IntegralInputs &in) {
    q.integral_relevance = quant16( (in.rel) * 1000.0 );
    q.integral_cost      = quant16( (in.cost) * 1000.0 );
    q.integral_physics   = quant16( (in.phys) * 1000.0 );
}

// If an HVNode-like type exists in the project, user can provide an overload
// hvlm::LBVH_helpers::to_qd(const std::vector<HVNode>&) elsewhere.
// Provide a weak template based on presence of HVNode:
template<typename HVNodeT>
inline std::vector<QDNode96> to_qd_from_hvnodes(const std::vector<HVNodeT>& nodes) {
    std::vector<QDNode96> out;
    out.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        QDNode96 q{};
        // best-effort mapping (requires HVNodeT to have morton or id fields)
        if constexpr (std::is_member_object_pointer_v<decltype(&HVNodeT::morton)>) {
            q.morton = nodes[i].morton;
        } else {
            q.morton = i;
        }
        // compute default integrals
        compute_integrals(q, IntegralInputs{1.0,1.0,1.0});
        q.skip_hint = 100;
        out.push_back(q);
    }
    return out;
}

// Compute trivial subtree ranges from nodes vector size
inline std::vector<std::pair<int,int>> compute_subtrees_from_size(size_t n) {
    return make_n_subtrees(n);
}

} // namespace LBVH_helpers
} // namespace hvlm
