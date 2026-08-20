cmake_minimum_required(VERSION 3.16)
project(IndexEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Include path ────────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include)

# ── Source files ────────────────────────────────────────────────────────
set(INDEX_SOURCES
    src/sorted_array_index.cpp
    src/chained_hash_index.cpp
    src/linear_probing_index.cpp
    src/extendible_hash_index.cpp
    src/bitmap_index.cpp
    src/btree_index.cpp
    src/bplus_tree_index.cpp
    src/rtree_index.cpp
)

# ── Static library ──────────────────────────────────────────────────────
add_library(index_engine STATIC ${INDEX_SOURCES})
target_include_directories(index_engine PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ── Test executable ─────────────────────────────────────────────────────
add_executable(test_all tests/test_all.cpp)
target_link_libraries(test_all PRIVATE index_engine)

# ── Benchmark executable (optimised) ───────────────────────────────────
add_executable(benchmark_all benchmarks/benchmark_all.cpp)
target_link_libraries(benchmark_all PRIVATE index_engine)
target_compile_options(benchmark_all PRIVATE -O2)
