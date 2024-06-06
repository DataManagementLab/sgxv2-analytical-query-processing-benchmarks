set(JOIN_SRCS
        src/util.cpp
        src/ChunkedTable.cpp
        src/cht/CHTJoinWrapper.cpp
        src/CrkJoin/JoinWrapper.cpp
        src/mway/joincommon.cpp
        src/mway/partition.cpp
        src/mway/scalar_multiwaymerge.cpp
        src/mway/scalarsort.cpp
        src/mway/sortmergejoin_multiway.cpp
        src/nl/nested_loop_join.cpp
        src/npj/HashLinkTableCommon.cpp
        src/npj/no_partitioning_bucket_chaining_join.cpp
        src/npj/no_partitioning_hash_join.cpp
        src/npj/no_partitioning_hash_join_st.cpp
        src/psm/parallel_sort.cpp
        src/psm/parallel_sortmerge_join.cpp
        src/psm/utility.cpp
        src/radix/radix_join.cpp
        src/radix/radix_sortmerge_join.cpp
        src/joins.cpp
)

add_library(joins STATIC ${JOIN_SRCS})
target_include_directories(joins PUBLIC include)
target_link_libraries(joins shared_headers x86_simd_sort logger ocalls-u Boost::boost)
target_compile_features(joins PUBLIC cxx_std_17)
target_compile_definitions(joins PRIVATE ${CFLAGS} CYCLES_PER_MICROSECOND=${CPMS})

add_library(joins-fq STATIC ${JOIN_SRCS})
target_include_directories(joins-fq PUBLIC include)
target_link_libraries(joins-fq shared_headers x86_simd_sort logger ocalls-u Boost::boost)
target_compile_features(joins-fq PUBLIC cxx_std_17)
target_compile_definitions(joins-fq PRIVATE ${CFLAGS} CYCLES_PER_MICROSECOND=${CPMS} FULL_QUERY)

add_trusted_library(joins-enclave SRCS ${JOIN_SRCS})
target_include_directories(joins-enclave PUBLIC include)
target_link_libraries(joins-enclave shared_headers x86_simd_sort logger-enclave ocalls-t Boost::boost)
target_compile_features(joins-enclave PUBLIC cxx_std_17)
target_compile_definitions(joins-enclave PRIVATE ${CFLAGS} CYCLES_PER_MICROSECOND=${CPMS})
