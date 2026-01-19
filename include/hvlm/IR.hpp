#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hvlm {

enum class Opcode : uint32_t {
    UNKNOWN = 0,
    BEAM = 1,
    SLAB = 2,
    LOAD = 3,
    FEM = 4,
    RENDER = 5
};

struct IRNode {
    Opcode op;
    std::vector<std::string> args;
};

using IR = std::vector<IRNode>;

} // namespace hvlm
