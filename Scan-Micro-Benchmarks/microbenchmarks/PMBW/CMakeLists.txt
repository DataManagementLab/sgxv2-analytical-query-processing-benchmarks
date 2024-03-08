set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -mavx -mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl -mavx2 -std=c++14")
#include_directories(${IPPCRYPTO_INCLUDE_DIRS} ${SGX_INCLUDE_DIR})
#link_libraries(${IPPCRYPTO_LIBRARIES})
set(EDL_SEARCH_PATHS Enclave)
set(E_SRCS Enclave/Enclave.cpp)
set(LDS Enclave/Enclave.lds)
add_enclave_library(pmbwenclave SRCS ${E_SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS} LDSCRIPT ${LDS})
target_link_libraries(pmbwenclave shared_headers)
target_compile_options(pmbwenclave PRIVATE -falign-functions=1024)
target_link_options(pmbwenclave PRIVATE -falign-functions=1024)
enclave_sign(pmbwenclave KEY Enclave/Enclave_private.pem CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp incl/funcs_x86_64.h Enclave/structs.h)
add_untrusted_executable(pmbw SRCS ${SRCS} EDL Enclave/Enclave.edl EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
target_include_directories(pmbw PRIVATE ${SGX_INCLUDE_DIR} incl Enclave)
target_compile_options(pmbw PRIVATE -falign-functions=1024)
target_link_options(pmbw PRIVATE -falign-functions=1024)
target_link_libraries(pmbw perfevent shared_headers gflags)
add_dependencies(pmbw pmbwenclave-sign)

configure_file(results/0_pmbw_linear.sh 0_pmbw_linear.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)

configure_file(results/1_pmbw_random.sh 1_pmbw_random.sh
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COPYONLY)
