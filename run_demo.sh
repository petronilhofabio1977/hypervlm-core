#!/usr/bin/env bash
set -euo pipefail
echo "Running DNA demo..."
if [ ! -d build ]; then
  mkdir -p build
  cd build
  cmake .. >/dev/null
  cmake --build . --parallel >/dev/null
  cd ..
fi
./build/dna_demo || echo "dna_demo not built"
./build/ddm_write_test || echo "ddm_write_test not built"
