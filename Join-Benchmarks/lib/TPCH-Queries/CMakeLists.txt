set(TPCH_SRCS
        src/result_transformers.cpp
        src/time_print.cpp
        src/tpch.cpp
)

add_library(tpch_queries STATIC ${TPCH_SRCS})
target_include_directories(tpch_queries PUBLIC include)
target_link_libraries(tpch_queries joins-fq)
target_compile_features(tpch_queries PUBLIC cxx_std_17)
target_compile_definitions(tpch_queries PRIVATE CYCLES_PER_MICROSECOND=${CPMS} ${CFLAGS})

add_trusted_library(tpch_queries_enclave SRCS ${TPCH_SRCS})
target_include_directories(tpch_queries_enclave PUBLIC include)
target_link_libraries(tpch_queries_enclave joins-enclave)
target_compile_features(tpch_queries_enclave PUBLIC cxx_std_17)
target_compile_definitions(tpch_queries_enclave PRIVATE CYCLES_PER_MICROSECOND=${CPMS} ${CFLAGS})
