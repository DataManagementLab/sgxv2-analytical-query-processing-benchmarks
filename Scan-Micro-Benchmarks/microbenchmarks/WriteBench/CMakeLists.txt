set(EDL_SEARCH_PATHS Enclave)
set(E_SRCS Enclave/Enclave.cpp shared/algorithms.cpp)

add_enclave_library(writebenchenc SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT Enclave/Enclave.lds)
target_link_libraries(writebenchenc shared_headers simd512scan-enclave)
target_compile_features(writebenchenc PUBLIC cxx_std_17)
target_compile_options(writebenchenc PUBLIC -fno-builtin -fno-builtin-memcpy)
enclave_sign(writebenchenc KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp App/OCalls.cpp App/Logger.cpp shared/algorithms.cpp)
add_untrusted_executable(writebench SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(writebench PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(writebench perfevent shared_headers gflags::gflags simd512scan)
target_compile_features(writebench PUBLIC cxx_std_17)
target_compile_options(writebench PUBLIC -fno-builtin -fno-builtin-memcpy)
add_dependencies(writebench writebenchenc-sign)

# Copy experiment run scripts to the binary directory and make them executable
configure_file(results/0_write.sh 0_write.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
configure_file(results/1_increment.sh 1_increment.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
configure_file(results/2_read.sh 2_read.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
