#include "hvlm/HVNodeGen.hpp"
#include <cstdlib>

namespace hvlm {

static uint32_t quantize_length(const std::string& val){
    try{
        size_t p = val.find_first_not_of("0123456789.");
        double v = atof(val.substr(0,p).c_str());
        if(p!=std::string::npos){
            auto u = val.substr(p);
            if(u=="m")  return v*1000.0;
            if(u=="cm") return v*10.0;
            if(u=="mm") return v;
        }
        return v;
    }catch(...){ return 0; }
}

std::vector<HVNode96> build_hvnodes_from_ir(const IR& ir){
    std::vector<HVNode96> out;
    out.reserve(ir.size());

    for(auto &n : ir){
        HVNode96 h;
        if(n.op == Opcode::BEAM){
            for(auto &a : n.args){
                auto e = a.find('=');
                if(e==std::string::npos) continue;
                auto k = a.substr(0,e);
                auto v = a.substr(e+1);
                if(k=="span") h.arg0 = quantize_length(v);
                if(k=="thickness") h.arg1 = quantize_length(v);
            }
        }
        h.finalize();
        out.push_back(h);
    }

    return out;
}

} // namespace hvlm
