cmake_minimum_required(VERSION 3.16)
project(hypervlm C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Libcurl
find_package(CURL REQUIRED)

# ---------------- SAFE FILE INCLUDE SYSTEM ----------------
file(GLOB_RECURSE HVLM_SOURCES
    "src/**/*.cpp" "src/**/*.hpp"
    "atm_v2/**/*.cpp" "atm_v2/**/*.hpp"
    "atm_bridge/**/*.cpp" "atm_bridge/**/*.hpp"
)
foreach(f IN LISTS HVLM_SOURCES)
    get_filename_component(ext "${f}" EXT)
    if(NOT ext STREQUAL ".cpp" AND NOT ext STREQUAL ".hpp")
        list(REMOVE_ITEM HVLM_SOURCES "${f}")
    endif()
endforeach()
# -----------------------------------------------------------


# --- HyperVLM extra targets for quick build ---
add_library(hvlm_qdnode INTERFACE)
target_include_directories(hvlm_qdnode INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_executable(run_renderer src/hvlm/render/renderer.cpp src/hvlm/render/integrator_path.hpp atm_bridge/src/ATMBridge.cpp src/hvlm/vm/diamond_vm.cpp src/hvlm/bvh/lbvh_builder.cpp)
target_compile_definitions(run_renderer PRIVATE RENDER_MAIN)
target_include_directories(run_renderer PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/atm_bridge/include)
target_compile_features(run_renderer PRIVATE cxx_std_17)

# --- Path tracer PBR target ---
add_executable(run_pathtracer
    src/hvlm/render_pt/main.cpp
    src/hvlm/render_pt/pathtracer.cpp
    src/hvlm/render_pt/bvh.hpp
    src/hvlm/render_pt/scene_geo.hpp
    atm_bridge/src/ATMBridge.cpp
)
target_include_directories(run_pathtracer PRIVATE ${CMAKE_SOURCE_DIR}/atm_bridge/include ${CMAKE_SOURCE_DIR}/src/hvlm/render_pt ${CMAKE_SOURCE_DIR}/include)

# --- Path tracer FULL (MIS + DOF + Denoise + OBJ + spectral approx) ---
add_executable(run_prod
    src/hvlm/render_prod/main.cpp
    src/hvlm/render_prod/integrator.cpp
    src/hvlm/render_prod/util.hpp
    src/hvlm/render_prod/geometry.hpp
    src/hvlm/render_prod/bvh.hpp
    src/hvlm/render_prod/material.hpp
    src/hvlm/render_prod/denoise.hpp
)
target_include_directories(run_prod PRIVATE ${CMAKE_SOURCE_DIR}/src/hvlm/render_prod ${CMAKE_SOURCE_DIR}/include)

# ---- render_engine (advanced) ----
add_executable(run_engine
    src/hvlm/render_engine/main.cpp
    src/hvlm/render_engine/integrator.cpp
    src/hvlm/render_engine/util.hpp
    src/hvlm/render_engine/geometry.hpp
    src/hvlm/render_engine/sah_bvh.hpp
    src/hvlm/render_engine/material.hpp
    src/hvlm/render_engine/denoise.hpp
    src/hvlm/render_engine/atm_integration.hpp
)
target_include_directories(run_engine PRIVATE ${CMAKE_SOURCE_DIR}/src/hvlm/render_engine ${CMAKE_SOURCE_DIR}/include)
# If you want to enable ATMBridge integration, add:
# target_compile_definitions(run_engine PRIVATE -DUSE_ATM_BRIDGE)
# target_include_directories(run_engine PRIVATE ${CMAKE_SOURCE_DIR}/atm_bridge/include)
