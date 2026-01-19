add_library(hvlm_core
    core/Parser.cpp
    core/IRBuilder.cpp
    core/HVNodeGen.cpp
)

target_include_directories(hvlm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_compile_features(hvlm_core PUBLIC cxx_std_23)
target_sources(hvlm_core PRIVATE
    runtime/DRE.cpp
    runtime/VM.cpp
)
target_sources(hvlm_core PRIVATE runtime/DRE.cpp)
