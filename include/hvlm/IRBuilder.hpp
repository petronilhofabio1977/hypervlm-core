#pragma once
#include "hvlm/AST.hpp"
#include "hvlm/IR.hpp"

namespace hvlm {
void build_ir_from_ast(const AST& ast, IR& out);
}
