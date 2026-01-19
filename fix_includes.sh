#!/usr/bin/env bash
set -euo pipefail
echo "Fixing includes to <dna/...> style..."
for f in src/dna/src/*.cpp src/dna/src/tests/*.cpp; do
  sed -i.bak -e 's|"util.hpp"|<dna/util.hpp>|g' \
              -e 's|"wal.hpp"|<dna/wal.hpp>|g' \
              -e 's|"dna_ddm.hpp"|<dna/dna_ddm.hpp>|g' \
              -e 's|"cluster_store.hpp"|<dna/cluster_store.hpp>|g' \
              -e 's|"virtual_cpu.hpp"|<dna/virtual_cpu.hpp>|g' "$f" || true
done
echo "done."
