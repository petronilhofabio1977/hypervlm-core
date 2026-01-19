#include "diamond_vm.hpp"
#include <iostream>

void DiamondVM::run(){
    std::cout << "[DiamondVM] executando " << nodes.size() << " nodos...\n";
    for(size_t i=0;i<nodes.size();i++){
        const auto& n = nodes[i];
        if(n.crc16 != crc16_compute(&n, sizeof(QDNode96))) {
            std::cerr << "CRC FAIL no node " << i << "\n";
        }
    }
    std::cout << "[DiamondVM] fim.\n";
}
