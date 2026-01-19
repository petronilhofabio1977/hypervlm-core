#!/usr/bin/env bash
set -euo pipefail
CXX=${CXX:-g++}
CXXFLAGS="-std=c++23 -O3 -march=native -pthread"
mkdir -p bin
$CXX $CXXFLAGS -Iinclude src/atm.cpp -c -o bin/atm.o
$CXX $CXXFLAGS -Iinclude bin/atm.o src/bench_math.cpp -o bin/bench_math
echo "Build concluído."
