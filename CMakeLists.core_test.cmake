# =========================
# CORE ONLY BUILD (TEST)
# =========================

add_library(hvlm_core STATIC
    src/hvlm/core/HVNodeGen.cpp
    src/hvlm/core/IRBuilder.cpp
    src/hvlm/core/Lexer.cpp
    src/hvlm/core/Parser.cpp
    src/hvlm/runtime/DRE.cpp
    src/hvlm/runtime/VM.cpp
    src/hvlm/vm/diamond_vm.cpp
)

target_include_directories(hvlm_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

add_executable(test_core tools/hvlmc_core.cpp)
target_link_libraries(test_core PRIVATE hvlm_core)
