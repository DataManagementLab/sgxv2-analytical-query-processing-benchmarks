add_library(logger STATIC src/Logger.cpp)
target_include_directories(logger PUBLIC include)
target_link_libraries(logger shared_headers)
target_compile_features(logger PUBLIC cxx_std_17)
target_compile_definitions(logger PRIVATE CYCLES_PER_MICROSECOND=${CPMS})

add_trusted_library(logger-enclave SRCS src/Logger.cpp)
target_include_directories(logger-enclave PUBLIC include)
target_link_libraries(logger-enclave enclave-print-t shared_headers)
target_compile_features(logger-enclave PUBLIC cxx_std_17)
target_compile_definitions(logger-enclave PRIVATE CYCLES_PER_MICROSECOND=${CPMS})
