include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/hvlm
    ${CMAKE_SOURCE_DIR}/src/hvlm/bvh
)

add_executable(run_dre run_dre.cpp)
target_link_libraries(run_dre hvlm_core)

add_executable(benchmark_dre benchmark_dre.cpp)
target_link_libraries(benchmark_dre hvlm_core)
