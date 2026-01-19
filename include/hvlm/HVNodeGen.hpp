#pragma once
#include "hvlm/HVNode.hpp"
#include "hvlm/IR.hpp"
#include <vector>

namespace hvlm {
std::vector<HVNode96> build_hvnodes_from_ir(const IR& ir);
}
