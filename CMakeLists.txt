cmake_minimum_required(VERSION 3.20)

project(cpu_toy_benchmark VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This is the only supported experiment profile.  It deliberately does not
# select a CPU architecture, enable LTO, or enable fast-math.
set(BENCHMARK_BASELINE_FLAGS)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND BENCHMARK_BASELINE_FLAGS
        -O2
        -fno-vectorize
        -fno-slp-vectorize
        -fno-unroll-loops
        -fno-fast-math)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND BENCHMARK_BASELINE_FLAGS
        -O2
        -fno-tree-vectorize
        -fno-tree-slp-vectorize
        -fno-unroll-loops
        -fno-fast-math)
elseif(MSVC)
    # MSVC has no stable, generally available switch that disables all auto
    # vectorization. /fp:precise, no /GL, and no /arch selection keep this as
    # close as practical to the Clang baseline profile.
    list(APPEND BENCHMARK_BASELINE_FLAGS /O2 /fp:precise /GL-)
else()
    message(FATAL_ERROR "Unsupported C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

string(REPLACE ";" " " BENCHMARK_BASELINE_FLAGS_TEXT "${BENCHMARK_BASELINE_FLAGS}")
set(BENCHMARK_RECORDED_FLAGS "${CMAKE_CXX_FLAGS} ${BENCHMARK_BASELINE_FLAGS_TEXT}")
string(STRIP "${BENCHMARK_RECORDED_FLAGS}" BENCHMARK_RECORDED_FLAGS)

find_package(Git QUIET)
set(BENCHMARK_GIT_COMMIT "unavailable")
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE BENCHMARK_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT BENCHMARK_GIT_COMMIT)
        set(BENCHMARK_GIT_COMMIT "unavailable")
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/build_config.h"
    @ONLY)

add_executable(cpu_benchmark
    src/main.cpp
    src/platform.cpp
    src/benchmark_runner.cpp
    src/benchmarks/sieve.cpp
    src/benchmarks/quicksort.cpp
    src/benchmarks/matrix_mul.cpp
    src/benchmarks/memory_access.cpp)

target_include_directories(cpu_benchmark PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_options(cpu_benchmark PRIVATE ${BENCHMARK_BASELINE_FLAGS})

if(MSVC)
    target_compile_options(cpu_benchmark PRIVATE /W4 /permissive-)
else()
    target_compile_options(cpu_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "CPU benchmark profile: baseline")
message(STATUS "Recorded compiler flags: ${BENCHMARK_RECORDED_FLAGS}")
