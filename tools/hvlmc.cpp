#include <iostream>
#include "ai/ai_native_llama.hpp"
#include "ai/ai_client.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <vector>

#include "hvlm/Parser.hpp"
#include "hvlm/IRBuilder.hpp"
#include "hvlm/VM.hpp"
#include "hvlm/HVNodeGen.hpp"
#include "ai/ai_client.hpp"

// Evita conflito com clock()
using hires_clock = std::chrono::high_resolution_clock;

long compile_once(const std::string& path)
{
    auto t0 = hires_clock::now();

    std::ifstream ifs(path);
    if(!ifs.good()) return -1;

    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string src = buf.str();

    hvlm::AST ast;
    std::string err;
    if(!hvlm::Parser::parse(src, ast, err)) return -1;

    hvlm::IR ir;
    hvlm::build_ir_from_ast(ast, ir);

    auto nodes = hvlm::build_hvnodes_from_ir(ir);

    std::string outp = path + ".vlm";
    std::ofstream ofs(outp, std::ios::binary);

    uint32_t count = nodes.size();
    ofs.write((char*)&count, 4);
    ofs.write((char*)nodes.data(), nodes.size() * sizeof(nodes[0]));

    auto t1 = hires_clock::now();
    long micros = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    return micros;
}

int main(int argc, char** argv)
{
    if(argc >= 3 && std::string(argv[1]) == "--ai") {
        std::string prompt;
        for(int i=2;i<argc;i++){
            prompt += argv[i];
            prompt += " ";
        }

        auto r = hvlm_ai::query_ollama(prompt);
        if(!r.ok){
            std::cerr << "[AI ERROR] " << r.text << "\n";
            return 1;
        }

        std::cout << "\n===== RESPOSTA DA IA =====\n";
        std::cout << r.text << "\n";
        std::cout << "==========================\n\n";
        return 0;
    }

    if(argc < 2){
        std::cout << "uso: hvlmc <arquivo.hv> [--bench N]\n";
        std::cout << "uso: hvlmc --ai \"prompt aqui\"\n";
        return 0;
    }

    std::string path = argv[1];

    if(argc == 3 && std::string(argv[2]) == "--run") {
        long t = compile_once(path);
        if(t < 0){
            std::cerr << "erro na compilação\n";
            return 1;
        }

        hvlm::HyperVLM_VM vm;
        if(!vm.load(path + ".vlm")) {
            std::cerr << "erro ao carregar VM\n";
            return 1;
        }
        vm.run();
        return 0;
    }

    if(argc == 4 && std::string(argv[2]) == "--bench") {
        int N = atoi(argv[3]);
        if(N <= 0) return 1;

        std::vector<long> times;
        times.reserve(N);

        for(int i=0;i<N;i++){
            long t = compile_once(path);
            if(t >= 0) times.push_back(t);
        }

        long min = times[0], max = times[0], sum = 0;
        for(long t : times){
            if(t < min) min = t;
            if(t > max) max = t;
            sum += t;
        }

        double avg = double(sum) / times.size();
        double var = 0;
        for(long t : times) var += (t - avg) * (t - avg);
        var /= times.size();
        double dev = std::sqrt(var);

        std::cout << "\n===== HVLMC BENCH =====\n";
        std::cout << "execuções: " << N << "\n";
        std::cout << "mínimo: "   << min << " us\n";
        std::cout << "máximo: "   << max << " us\n";
        std::cout << "média : "   << avg << " us\n";
        std::cout << "desvio: "  << dev << " us\n";
        std::cout << "=======================\n";
        return 0;
    }

    long t = compile_once(path);
    std::cout << "[hvlmc] wrote " << path << ".vlm\n";
    std::cout << "[hvlmc] compile_time = " << t << " us\n";
    return 0;
}
