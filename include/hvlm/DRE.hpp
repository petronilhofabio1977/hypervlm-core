#pragma once
#include <vector>
#include <queue>
#include <optional>
#include "hvlm/QDNode96.hpp"

namespace hvlm {

struct DREConfig {
    double w_relevance = 1.0;
    double w_cost      = 1.0;
    double w_physics   = 1.0;
    double w_priority  = 1.0;
    double w_morton_loc = 0.001;
    uint8_t skip_threshold = 200;
};

class DRE {
public:
    struct Range {
        int l;
        int r;
        double score;
        Range(int L, int R, double S) : l(L), r(R), score(S) {}
    };

private:
    std::vector<QDNode96>& d;
    const std::vector<std::pair<int,int>>& subtree_ranges;
    DREConfig conf;

    struct Cmp {
        bool operator()(const Range& a, const Range& b) const {
            return a.score < b.score;
        }
    };

    std::priority_queue<Range, std::vector<Range>, Cmp> pq;

public:


    DRE(std::vector<QDNode96>& diamond, const std::vector<std::pair<int,int>>& subtree, DREConfig cfg = {});
    void init_root();

    std::optional<int> next_node();

    double score_of(int idx) const;

    double score_range(int l, int r) const;

    bool split_range(const Range& rg, Range& left, Range& right) const;

    void update_node_integrals(int idx, double rel, double cost, double phys);
};

} // namespace hvlm
