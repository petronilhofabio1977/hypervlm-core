#!/usr/bin/env bash
set -euo pipefail
CXX=${CXX:-g++}
CXXFLAGS="-std=c++23 -O3 -march=native -pthread -I../atm_v2/include -I../atm_v2 -I../atm_v2/include -I../atm_bridge/include"
# compile atm_v2 objects are expected in ../atm_v2/bin/*.o
# if atm_v2 was built via atm_v2/build.sh, we have bin/workqueue.o and bin/atm_v2.o
mkdir -p bin
echo "Compiling ATMBridge..."
$CXX $CXXFLAGS -Iinclude -c src/ATMBridge.cpp -o bin/ATMBridge.o
echo "Compiling integration test..."
$CXX $CXXFLAGS test/integration.cpp bin/ATMBridge.o ../atm_v2/bin/workqueue.o ../atm_v2/bin/atm_v2.o -o bin/integration -pthread
echo "Done. Run ./bin/integration"
