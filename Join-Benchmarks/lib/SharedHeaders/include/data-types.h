#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>

#ifdef KEY_8B /* 64-bit key/payload, 16B tuples */
typedef uint64_t intkey_t;
typedef uint64_t value_t;
#else /* 32-bit key/payload, 8B tuples */
typedef uint32_t intkey_t;
typedef uint32_t value_t;
#endif

#if !defined PRId64
# undef PRId64
# define PRId64 "lld"
#endif

#if !defined PRIu64
# undef PRIu64
# define PRIu64 "llu"
#endif

#ifndef B_TO_MB
#define B_TO_MB(X) ((double)X/1024.0/1024.0)
#endif

#ifndef __forceinline
#define __forceinline __attribute__((always_inline))
#endif

typedef uint32_t type_key;
typedef uint32_t type_value;

typedef struct row_t tuple_t;
typedef struct output_list_t output_list_t;
typedef struct output_t output_t;
typedef struct table_t relation_t;
typedef struct result_t result_t;
typedef struct threadresult_t threadresult_t;
typedef struct joinconfig_t joinconfig_t;
typedef struct join_result_t join_result_t;

struct row_t {
    type_key key;
    type_value payload;
};

struct table_t {
    struct row_t* tuples;
    uint64_t num_tuples;
    int ratio_holes;
    int sorted;
};

struct output_list_t {
    type_key key;
    type_value Rpayload;
    type_value Spayload;
    struct output_list_t * next;
};

struct output_t {
    output_list_t * list;
    uint64_t size;
};

struct output_triple_t {
    type_key key;
    type_value Rpayload;
    type_value Spayload;
};

#ifndef CSKB
#define CSKB 16
#endif

#define CHUNK_SIZE (1024 * CSKB)
#define TUPLES_PER_CHUNK ((CHUNK_SIZE - 8) / sizeof(struct output_triple_t))

struct table_chunk_t {
    uint64_t num_tuples;
    struct output_triple_t tuples[TUPLES_PER_CHUNK];
};

struct chunked_table_t {
    struct table_chunk_t **chunks; // Pointers to the chunks
    uint64_t current_chunk; // The chunk that is currently being filled
    uint64_t num_chunks; // The number of allocated chunks (non-null pointers in chunks)
    uint64_t chunk_capacity; // The number of pointer that can fit into chunks
    uint64_t num_tuples; // Total number of tuples over all chunks
};

struct algorithm_t {
    char name[128];
    result_t *  (*join)(const struct table_t*, const struct table_t*, const struct joinconfig_t*);
};

/** Holds the join results of a thread */
struct threadresult_t {
    int64_t  nresults;
    output_list_t *   results;
    uint32_t threadid;
};

/** Type definition for join results. */
struct result_t {
    int64_t                 totalresults;
    int                     nthreads;
    double                  throughput;
    int                     materialized;
    void *                  result;
    int                     result_type; // 0 = threadresult_t*, 1 = chunked_table_t*
};

struct join_result_t {
    uint64_t matches;
    uint64_t checksum;
    uint64_t time_usec;
    uint64_t part_usec;
    uint64_t join_usec;
    uint64_t join_build_usec;
    uint64_t join_partition_usec;
    uint64_t join_probe_usec;
    uint64_t timer_ht;
    uint64_t timer_tree;
    uint64_t timer_part;
    uint64_t timer_probe;
    uint64_t ** timer_per_partition;
};

struct timers_t {
    uint64_t total;
    uint64_t partition;
    uint64_t build;
    uint64_t probe;
    uint64_t timer4;
    uint64_t start;
    uint64_t end;
};

struct rusage_reduced_t {
    signed long ru_utime_sec;
    signed long ru_utime_usec;
    signed long ru_stime_sec;
    signed long ru_stime_usec;
    long ru_minflt;
    long ru_majflt;
    long ru_nvcsw;
    long ru_nivcsw;
};

/**
 * Various NUMA shuffling strategies for data shuffling phase of join
 * algorithms as also described by NUMA-aware data shuffling paper [CIDR'13].
 *
 * NUMA_SHUFFLE_RANDOM, NUMA_SHUFFLE_RING, NUMA_SHUFFLE_NEXT
 */
enum numa_strategy_t {RANDOM, RING, NEXT};

/** Join configuration parameters. */
struct joinconfig_t {
    int NTHREADS;
    int PARTFANOUT;
    int SCALARSORT;
    int SCALARMERGE;
    int MWAYMERGEBUFFERSIZE;
    enum numa_strategy_t NUMASTRATEGY;

    int RADIXBITS;
    int WRITETOFILE;
    int MATERIALIZE;
    int PRINT;
    int CRACKING_THRESHOLD;
    int ALLOC_CORE;
};

typedef struct __attribute__((aligned(64))) {
    unsigned int thread_num;
    unsigned long num_elements;
    unsigned long int idx_i;
    unsigned long* histogram;
    tuple_t* data;
} chunk;

typedef struct __attribute__((aligned(64))) {
    unsigned int thread_num;
    long long int start;
    long long int end;
    long long int idx_i;
    long long int idx_j;
} info_per_thread;

#ifndef MAXLOADFACTOR
#define MAXLOADFACTOR 1.1
#endif

#endif //DATA_TYPES_H
