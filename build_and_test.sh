#!/usr/bin/env bash
set -euo pipefail
echo "BUILD & TEST (full) starting..."
mkdir -p build
cd build
cmake .. | tee cmake_output.log
cmake --build . --parallel | tee build_output.log
ctest --output-on-failure | tee test_output.log
echo "BUILD+TEST finished."
