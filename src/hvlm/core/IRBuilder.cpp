#include "hvlm/IRBuilder.hpp"

namespace hvlm {

static Opcode opcode_from_name(const std::string& n){
    if(n=="beam") return Opcode::BEAM;
    if(n=="slab") return Opcode::SLAB;
    if(n=="fem") return Opcode::FEM;
    if(n=="render") return Opcode::RENDER;
    return Opcode::UNKNOWN;
}

void build_ir_from_ast(const AST& ast, IR& out){
    out.clear();
    for(auto &o : ast.objects){
        IRNode n;
        n.op = opcode_from_name(o.name);
        for(auto &p : o.props){
            n.args.push_back(p.key + "=" + p.value);
        }
        out.push_back(n);
    }
}

} // namespace hvlm
