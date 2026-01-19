#include <cstdint>
#include <cmath>
#include <iostream>
#include <optional>

#include "hvlm/DRE.hpp"
#include "hvlm/QDNode96.hpp"

namespace hvlm {

static uint16_t clamp_u16(double v) {
    if (v < 0) return 0;
    if (v > 65535) return 65535;
    return (uint16_t)v;
}

DRE::DRE(std::vector<QDNode96>& diamond,
         const std::vector<std::pair<int,int>>& subtree,
         DREConfig cfg)
    : d(diamond), subtree_ranges(subtree), conf(cfg)
{
}

void DRE::init_root() {
    while (!pq.empty()) pq.pop();

    for (auto &p : subtree_ranges) {
        if (p.first <= p.second) {
            pq.push(Range{p.first, p.second, score_range(p.first, p.second)});
        }
    }

    if (pq.empty() && !d.empty()) {
        pq.push(Range{0, (int)d.size()-1, score_range(0, (int)d.size()-1)});
    }
}

double DRE::score_of(int idx) const {
    if (idx < 0 || idx >= (int)d.size()) return 0.0;

    const auto &n = d[idx];

    double rel  = double(n.integral_relevance);
    double cost = double(n.integral_cost);
    double phys = double(n.integral_physics);

    double denom = rel + cost + phys + 1.0;

    double s_rel  = rel / denom;
    double s_cost = 1.0 - cost / denom;
    double s_phys = phys / denom;

    double p_hint = double(n.priority_hint);
    double skip   = double(n.skip_hint);

    double morton_loc = 1.0 / (1.0 + std::abs(double(n.morton)) * conf.w_morton_loc);

    double score =
        conf.w_relevance * s_rel +
        conf.w_cost      * s_cost +
        conf.w_physics   * s_phys +
        conf.w_priority  * (p_hint / 65535.0) +
        morton_loc;

    if (n.skip_hint < conf.skip_threshold)
        score *= 1.05;

    return score;
}

double DRE::score_range(int l, int r) const {
    if (l > r) return 0.0;
    return score_of((l + r) / 2);
}

bool DRE::split_range(const DRE::Range& rg,
                      DRE::Range& left,
                      DRE::Range& right) const
{
    if (rg.l >= rg.r) return false;

    int mid = (rg.l + rg.r) / 2;

    left  = Range{rg.l, mid, score_range(rg.l, mid)};
    right = Range{mid+1, rg.r, score_range(mid+1, rg.r)};

    return true;
}

std::optional<int> DRE::next_node() {
    while (!pq.empty()) {

        Range top = pq.top();
        pq.pop();

        int mid = (top.l + top.r) / 2;
        if (mid < top.l || mid > top.r)
            continue;

        auto &node = d[mid];

        if (node.skip_hint >= conf.skip_threshold) {
            Range L{0,0,0.0}, R{0,0,0.0};
            if (split_range(top, L, R)) {
                pq.push(R);
                pq.push(L);
                continue;
            }
        }

        node.skip_hint = uint8_t(std::max(0, int(node.skip_hint) - 5));

        Range L{0,0,0.0}, R{0,0,0.0};
        if (split_range(top, L, R)) {
            pq.push(R);
            pq.push(L);
        }

        return mid;
    }

    return std::nullopt;
}

void DRE::update_node_integrals(int idx,
                                double rel,
                                double cost,
                                double phys)
{
    if (idx < 0 || idx >= (int)d.size())
        return;

    d[idx].integral_relevance = clamp_u16(rel);
    d[idx].integral_cost      = clamp_u16(cost);
    d[idx].integral_physics   = clamp_u16(phys);
}

} // namespace hvlm

