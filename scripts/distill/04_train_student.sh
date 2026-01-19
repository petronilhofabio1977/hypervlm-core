#!/usr/bin/env bash
# Copia tiny.txt para build/ (run_train espera arquivo no cwd build) e roda run_train
set -e
BUILD_DIR="build"
SRC_TINY="tiny.txt"
if [ ! -f "$SRC_TINY" ]; then
    echo "ERRO: $SRC_TINY não encontrado. Rode os passos anteriores."
    exit 1
fi
cp "$SRC_TINY" "$BUILD_DIR/tiny.txt"
pushd "$BUILD_DIR" >/dev/null
echo "Rodando run_train em $(pwd)"
./run_train || { echo "run_train falhou"; popd >/dev/null; exit 1; }
popd >/dev/null
