set(EDL_SEARCH_PATHS Enclave)
set(E_SRCS Enclave/Enclave.cpp
        Shared/partitioning_algorithms.cpp
        Shared/histogram_algorithms.cpp
        Shared/simple_algorithms.cpp
        Shared/RNG.cpp)

add_enclave_library(radixpartenc SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT Enclave/Enclave.lds)
target_link_libraries(radixpartenc shared_headers)
target_compile_features(radixpartenc PUBLIC cxx_std_17)
target_compile_options(radixpartenc PRIVATE -falign-functions=1024 -march=native)
target_link_options(radixpartenc PRIVATE -falign-functions=1024)
enclave_sign(radixpartenc KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp App/OCalls.cpp App/Logger.cpp Shared/partitioning_algorithms.cpp Shared/histogram_algorithms.cpp)
add_untrusted_executable(radixpart SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(radixpart PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(radixpart perfevent shared_headers gflags::gflags)
target_compile_features(radixpart PUBLIC cxx_std_17)
target_compile_options(radixpart PRIVATE -falign-functions=1024 -march=native)
target_link_options(radixpart PRIVATE -falign-functions=1024)
add_dependencies(radixpart radixpartenc-sign)

set(SRCS App/Histogram.cpp App/OCalls.cpp App/Logger.cpp Shared/histogram_algorithms.cpp)
add_untrusted_executable(histogram SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(histogram PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(histogram perfevent shared_headers gflags::gflags)
target_compile_features(histogram PUBLIC cxx_std_17)
target_compile_options(histogram PRIVATE -falign-functions=1024 -march=native)
target_link_options(histogram PRIVATE -falign-functions=1024)
add_dependencies(histogram radixpartenc-sign)

set(SRCS App/Simple.cpp App/OCalls.cpp App/Logger.cpp Shared/simple_algorithms.cpp Shared/RNG.cpp)
add_untrusted_executable(simple SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(simple PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(simple perfevent shared_headers gflags::gflags)
target_compile_features(simple PUBLIC cxx_std_17)
target_compile_options(simple PRIVATE -falign-functions=1024 -march=native)
target_link_options(simple PRIVATE -falign-functions=1024)
add_dependencies(simple radixpartenc-sign)

configure_file(results/0_histogram.sh 0_histogram.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)

configure_file(results/1_simple.sh 1_simple.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
