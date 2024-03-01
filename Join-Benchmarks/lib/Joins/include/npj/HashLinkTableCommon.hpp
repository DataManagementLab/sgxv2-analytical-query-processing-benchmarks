#ifndef JOIN_BENCH_MATTHIAS_HASHLINKTABLE_HPP
#define JOIN_BENCH_MATTHIAS_HASHLINKTABLE_HPP

#include <cstdlib>
#include <cstring>
#include "npj_types.h"
#include "npj_params.h"
#include "Logger.hpp"
#include "lock.h"

#ifndef ENCLAVE
#include <malloc.h>
#include "ocalls.hpp"
#endif

#include "rdtscpWrapper.h"

#ifndef NEXT_POW_2
/**
 *  compute the next number, greater than or equal to 32-bit unsigned v.
 *  taken from "bit twiddling hacks":
 *  http://graphics.stanford.edu/~seander/bithacks.html
 */
#define NEXT_POW_2(V)                           \
    do {                                        \
        V--;                                    \
        V |= V >> 1;                            \
        V |= V >> 2;                            \
        V |= V >> 4;                            \
        V |= V >> 8;                            \
        V |= V >> 16;                           \
        V++;                                    \
    } while(0)
#endif

#ifndef HASH
#define HASH(X, MASK, SKIP) (((X) & MASK) >> SKIP)
#endif

void
print_timing(uint64_t start, uint64_t end, uint64_t total, uint64_t build, uint64_t probe,
             uint64_t numtuples, int64_t result);

/**
 * Allocate a new hash table and saves the pointer to it in ppht. nbuckets is rounded up to the next power of 2.
 * @param ppht
 * @param nbuckets
 */
void
allocate_hashtable(hashtable_t **ppht, uint32_t nbuckets);

/**
 * Releases memory allocated for the hashtable.
 *
 * @param ht pointer to hashtable
 */
void
destroy_hashtable(hashtable_t *ht);

/**
 * Probes the hashtable for the given outer relation, returns num results.
 * This probing method is used for both single and multi-threaded version.
 *
 * @param ht hashtable to be probed
 * @param rel the probing outer relation
 *
 * @return number of matching tuples
 */
int64_t
probe_hashtable(const hashtable_t *ht, const struct table_t *rel, output_list_t **output, int materialize);

/**
 * Probes the hashtable for the given outer relation, returns num results.
 * Cannot deal with overflow buckets
 *
 * @param ht hash table to be probed
 * @param rel the probing outer relation
 *
 * @return number of matching tuples
 */
[[nodiscard]] int64_t
probe_hashtable_no_overflow(const hashtable_t *ht, const struct table_t *rel);

/**
 * Probes the hashtable for the given outer relation, returns num results.
 * Cannot deal with overflow buckets. And uses unrolling to improve performance. Optimization for counting matches only
 * works because of the maximum bucket size.
 *
 * @param ht hash table to be probed
 * @param rel the probing outer relation
 *
 * @return number of matching tuples
 */
[[nodiscard]] int64_t
probe_hashtable_no_overflow_unrolled(const hashtable_t *ht, const struct table_t *rel);

/**
 * Initializes a new bucket_buffer_t for later use in allocating
 * buckets when overflow occurs.
 *
 * @param ppbuf [in,out] bucket buffer to be initialized
 */
void init_bucket_buffer(bucket_buffer_t **ppbuf);

/**
 * Returns a new bucket_t from the given bucket_buffer_t.
 * If the bucket_buffer_t does not have enough space, then allocates
 * a new bucket_buffer_t and adds to the list.
 *
 * @param result [out] the new bucket
 * @param buf [in,out] the pointer to the bucket_buffer_t pointer
 */
void get_new_bucket(bucket_t **result, bucket_buffer_t **buf);

/** De-allocates all the bucket_buffer_t */
void free_bucket_buffer(bucket_buffer_t *buf);

#endif //JOIN_BENCH_MATTHIAS_HASHLINKTABLE_HPP
