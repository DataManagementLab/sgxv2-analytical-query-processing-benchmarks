set(EDL_SEARCH_PATHS Enclave)
set(E_SRCS Enclave/Enclave.cpp Shared/algorithms.cpp)

add_enclave_library(sortenc SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT Enclave/Enclave.lds)
target_link_libraries(sortenc shared_headers simd512scan-enclave)
target_compile_features(sortenc PUBLIC cxx_std_17)
enclave_sign(sortenc KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp App/OCalls.cpp App/Logger.cpp Shared/algorithms.cpp)
add_untrusted_executable(sort SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(sort PRIVATE ${SGX_INCLUDE_DIR})
target_link_libraries(sort perfevent shared_headers gflags::gflags simd512scan)
target_compile_features(sort PUBLIC cxx_std_17)
add_dependencies(sort sortenc-sign)
