file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB SIMD_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/simd-headers/*.h")

add_library(simd512scan STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/SIMD512.cpp ${HEADER_LIST})
target_include_directories(simd512scan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(simd512scan PUBLIC cxx_std_14)
target_link_libraries(simd512scan PRIVATE shared_headers)
set(SIMD_SCAN_FLAGS "${APP_CXX_FLAGS} -mavx -mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl -mavx2 -mavx512vbmi -mavx512vbmi2")
set_target_properties(simd512scan PROPERTIES COMPILE_FLAGS ${SIMD_SCAN_FLAGS})

add_trusted_library(simd512scan-enclave SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/SIMD512.cpp ${HEADER_LIST} ${SIMD_HEADERS})
target_include_directories(simd512scan-enclave PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(simd512scan-enclave PUBLIC cxx_std_14)
target_link_libraries(simd512scan-enclave shared_headers)
target_compile_options(simd512scan-enclave PRIVATE -mavx -mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl -mavx2 -mavx512vbmi -mavx512vbmi2)

include(CTest)

add_subdirectory(tests)