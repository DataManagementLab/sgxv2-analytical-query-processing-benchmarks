cmake_minimum_required(VERSION 3.16)
project(sgxv2-join-benchmarks)

# FetchContent added in CMake 3.11, downloads during the configure step
include(FetchContent)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
# declare dependencies
find_package(SGX REQUIRED) # sgx sdk
set(Boost_ROOT "/opt/boost_1_84_0" CACHE STRING "Boost directory")
set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not use system boost")
find_package(Boost REQUIRED)

# globally set flags for all compiler calls
add_compile_options(-m64 -march=native -fvisibility=hidden -fstack-protector-strong -g)
set(ENCLAVE_CONFIG_FILE "Enclave/Enclave4GB.config.xml" CACHE STRING "Path to the enclave config")
set(CPMS "2900" CACHE STRING "Number of CPU cycles mer micro second (not! milli second!)")
set(CMAKE_CXX_STANDARD 17)

# create join library
add_subdirectory(lib)

add_executable(csv_convert App/TpcH/CSVConvert.cpp App/TpcH/TpcHCommons.cpp)
target_link_libraries(csv_convert logger shared_headers)
target_compile_features(csv_convert PUBLIC cxx_std_20)

configure_file(App/TpcH/create_binary_tables.sh create_binary_tables.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)

# create enclave
set(EDL_SEARCH_PATHS Enclave lib/EnclavePrint/interface lib/OCalls/interface)
set(E_SRCS Enclave/secure_joins.cpp Enclave/TpcHECalls.cpp)
set(LDS Enclave/Enclave.lds)
add_enclave_library(joinenclave
        SRCS ${E_SRCS}
        EDL Enclave/Enclave.edl
        EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS}
        LDSCRIPT ${LDS}
        EDL_DEPENDENCIES shared_headers)
target_link_libraries(joinenclave tpch_queries_enclave shared_headers)
#target_include_directories(joinenclave PRIVATE Include lib/x86-simd-sort)
target_compile_features(joinenclave PUBLIC cxx_std_17)
enclave_sign(joinenclave KEY Enclave/Enclave_private.pem CONFIG ${ENCLAVE_CONFIG_FILE})

# Create TEEBench binary
set(TEEBENCH_SRCS App/TEEBench/App.cpp)
add_untrusted_executable(teebench SRCS ${TEEBENCH_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(teebench PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(teebench app_utilities enclave-print-u ocalls-u)
target_compile_features(teebench PUBLIC cxx_std_17)
target_compile_definitions(teebench PRIVATE CYCLES_PER_MICROSECOND=${CPMS})
add_dependencies(teebench joinenclave-sign)

# Create TEEBench native binary
set(TEEBENCH_NATIVE_SRCS App/TEEBench/native.cpp)
add_executable(native ${TEEBENCH_NATIVE_SRCS})
target_link_libraries(native joins app_utilities)
target_compile_features(native PUBLIC cxx_std_17)
target_compile_definitions(native PRIVATE CYCLES_PER_MICROSECOND=${CPMS})

# Create TPC-H binary
set(TPCH_SRCS App/TpcH/TpcHApp.cpp App/TpcH/TpcHCommons.cpp)
add_untrusted_executable(tpch SRCS ${TPCH_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(tpch PRIVATE ${SGX_INCLUDE_DIR} App/TpcH)
target_link_libraries(tpch app_utilities logger enclave-print-u ocalls-u)
target_compile_features(tpch PUBLIC cxx_std_17)
target_compile_definitions(tpch PRIVATE CYCLES_PER_MICROSECOND=${CPMS})
add_dependencies(tpch joinenclave-sign)

# Create TPC-H native binary
set(TPCH_NATIVE_SRCS App/TpcH/TpcHNative.cpp App/TpcH/TpcHCommons.cpp)
add_executable(tpch-native ${TPCH_NATIVE_SRCS})
target_link_libraries(tpch-native app_utilities tpch_queries logger)
target_compile_features(tpch-native PUBLIC cxx_std_17)
target_compile_definitions(tpch-native PRIVATE CYCLES_PER_MICROSECOND=${CPMS})
