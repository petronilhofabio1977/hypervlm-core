#include "morton3d.hpp"
#include <vector>
#include <algorithm>
#include "../../../include/qdnode/qdnode.hpp"

std::vector<QDNode96> build_diamond_bvh(std::vector<QDNode96> nodes){
    std::sort(nodes.begin(), nodes.end(),
        [](const QDNode96&a, const QDNode96&b){return a.morton < b.morton;});
    return nodes;
}
