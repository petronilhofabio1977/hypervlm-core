#!/usr/bin/env bash
set -e

FILE="src/dna/CMakeLists.txt"

echo ">>> Corrigindo predictor_smoke_test em $FILE"

# Remove bloco antigo se existir
sed -i '/predictor_smoke_test/,+10d' "$FILE"

# Adiciona bloco correto
cat >> "$FILE" <<'CM'

# --- Predictor smoke test (gshare + perceptron + manager)
add_executable(predictor_smoke_test
    src/predictor_manager.cpp
    src/gshare.cpp
    src/perceptron.cpp
    src/tests/gshare_test.cpp
)
target_include_directories(predictor_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (TARGET dna)
    target_link_libraries(predictor_smoke_test PRIVATE dna)
endif()
add_test(NAME predictor_smoke_test COMMAND predictor_smoke_test)

CM

echo ">>> Feito."
