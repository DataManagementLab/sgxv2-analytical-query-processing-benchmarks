cmake_minimum_required(VERSION 3.14)
project(sgxv2-benchmarks)

# FetchContent added in CMake 3.11, downloads during the configure step
include(FetchContent)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
# declare dependencies
find_package(SGX REQUIRED) # sgx sdk

add_subdirectory(shared_libraries)
add_subdirectory(microbenchmarks)
