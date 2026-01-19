#include <iostream>
#include <cstdint>

int main() {
    size_t tile_size = 1024 * 1024; // 1 MB por tile
    int tiles_dim = 1024;           // 1024×1024 tiles
    int lods = 10;                  // 10 LODs

    double total_TB =
        (double)tiles_dim * tiles_dim * lods * tile_size /
        (1024.0 * 1024.0 * 1024.0 * 1024.0);

    std::cout << "\n=== SIMULAÇÃO DE MEMÓRIA VIRTUAL ===\n\n";
    std::cout << "Tiles simulados: " << (tiles_dim * tiles_dim * lods) << "\n";
    std::cout << "Tamanho por tile: " << tile_size << " bytes (1 MB)\n";
    std::cout << "Total simulado: " << total_TB << " TB\n\n";
    std::cout << "Nenhum dado real gerado.\n";
    std::cout << "Nenhum shard escrito.\n";
    std::cout << "Simulação concluída instantaneamente.\n\n";

    return 0;
}
