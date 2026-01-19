#!/usr/bin/env bash
set -euo pipefail

CXX=${CXX:-g++}
CXXFLAGS="-std=c++23 -O3 -march=native -pthread"

mkdir -p bin

echo "[1/3] Compilando WorkQueue..."
$CXX $CXXFLAGS -Iinclude src/workqueue.cpp -c -o bin/workqueue.o

echo "[2/3] Compilando ATMv2..."
$CXX $CXXFLAGS -Iinclude src/atm_v2.cpp -c -o bin/atm_v2.o

echo "[3/3] Ligando biblioteca ATMv2..."
$CXX $CXXFLAGS -Iinclude bin/workqueue.o bin/atm_v2.o -o bin/atm_v2

echo "-------------------------------------"
echo "ATM v2 compilado com sucesso!"
echo "Binário: ./bin/atm_v2"
echo "-------------------------------------"
