#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>

#include "hvlm/Parser.hpp"
#include "hvlm/IRBuilder.hpp"
#include "hvlm/HVNodeGen.hpp"
#include "hvlm/VM.hpp"

using hires_clock = std::chrono::high_resolution_clock;

long compile_once(const std::string& path)
{
    auto t0 = hires_clock::now();

    std::ifstream ifs(path);
    if(!ifs.good()){
        std::cerr << "erro ao abrir arquivo\n";
        return -1;
    }

    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string src = buf.str();

    hvlm::AST ast;
    std::string err;
    if(!hvlm::Parser::parse(src, ast, err)){
        std::cerr << "erro de parse: " << err << "\n";
        return -1;
    }

    hvlm::IR ir;
    hvlm::build_ir_from_ast(ast, ir);

    auto nodes = hvlm::build_hvnodes_from_ir(ir);

    std::string outp = path + ".vlm";
    std::ofstream ofs(outp, std::ios::binary);

    uint32_t count = nodes.size();
    ofs.write((char*)&count, sizeof(count));
    ofs.write((char*)nodes.data(), nodes.size() * sizeof(nodes[0]));

    auto t1 = hires_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

int main(int argc, char** argv)
{
    if(argc < 2){
        std::cout << "uso: hvlmc <arquivo.hv> [--run]\n";
        return 0;
    }

    std::string path = argv[1];

    if(argc == 3 && std::string(argv[2]) == "--run"){
        long t = compile_once(path);
        if(t < 0) return 1;

        hvlm::HyperVLM_VM vm;
        if(!vm.load(path + ".vlm")){
            std::cerr << "erro ao carregar VM\n";
            return 1;
        }
        vm.run();
        return 0;
    }

    long t = compile_once(path);
    if(t < 0) return 1;

    std::cout << "[hvlmc] wrote " << path << ".vlm\n";
    std::cout << "[hvlmc] compile_time = " << t << " us\n";
    return 0;
}
