add_trusted_library(enclave-print-t HEADER_ONLY SRCS trusted/src/EnclavePrint.cpp)
target_include_directories(enclave-print-t PUBLIC trusted/include)
target_compile_features(enclave-print-t PUBLIC cxx_std_17)

add_library(enclave-print-u STATIC untrusted/src/print_ocalls.cpp)
target_include_directories(enclave-print-u PUBLIC untrusted/include)
target_compile_features(enclave-print-u PUBLIC cxx_std_17)
