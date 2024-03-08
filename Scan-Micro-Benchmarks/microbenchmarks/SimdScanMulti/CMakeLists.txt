set(EDL_SEARCH_PATHS Enclave)
set(E_SRCS Enclave/Enclave.cpp)
set(LDS Enclave/Enclave.lds)
add_enclave_library(simdmultienclave SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT ${LDS})
target_link_libraries(simdmultienclave shared_headers simd512scan-enclave)
target_compile_features(simdmultienclave PUBLIC cxx_std_17)
enclave_sign(simdmultienclave KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp App/multithreadedscan.cpp App/multithreadscan_utils.cpp App/OCalls.cpp App/types.cpp App/Logger.cpp)
add_untrusted_executable(simdmulti SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(simdmulti PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(simdmulti perfevent gflags::gflags simd512scan shared_headers)
target_compile_features(simdmulti PUBLIC cxx_std_17)
add_dependencies(simdmulti simdmultienclave-sign)

# Copy experiment run scripts to the binary directory and make them executable
configure_file(results/0_single_thread.sh 0_single_thread.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
configure_file(results/1_scale_up.sh 1_scale_up.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
configure_file(results/2_write_rate.sh 2_write_rate.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
configure_file(results/3_numa.sh 3_numa.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
