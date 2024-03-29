set(EDL_SEARCH_PATHS Enclave)
set(E_SRCS Enclave/Enclave.cpp Shared/algorithms.cpp)

add_enclave_library(radixpartenc SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT Enclave/Enclave.lds)
target_link_libraries(radixpartenc shared_headers simd512scan-enclave)
target_compile_features(radixpartenc PUBLIC cxx_std_17)
target_compile_options(radixpartenc PRIVATE -falign-functions=1024)
target_link_options(radixpartenc PRIVATE -falign-functions=1024)
enclave_sign(radixpartenc KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp App/OCalls.cpp App/Logger.cpp Shared/algorithms.cpp)
add_untrusted_executable(radixpart SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(radixpart PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(radixpart perfevent shared_headers gflags::gflags simd512scan)
target_compile_features(radixpart PUBLIC cxx_std_17)
target_compile_options(radixpart PRIVATE -falign-functions=1024)
target_link_options(radixpart PRIVATE -falign-functions=1024)
add_dependencies(radixpart radixpartenc-sign)

configure_file(results/0_histogram.sh 0_histogram.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
