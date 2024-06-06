#ifndef JOIN_BENCH_MATTHIAS_NO_PARTITIONING_BUCKET_CHAINING_JOIN_HPP
#define JOIN_BENCH_MATTHIAS_NO_PARTITIONING_BUCKET_CHAINING_JOIN_HPP

#include "data-types.h"

result_t*
NPBC_st(const table_t *relR, const table_t *relS, const joinconfig_t *config);

#endif //JOIN_BENCH_MATTHIAS_NO_PARTITIONING_BUCKET_CHAINING_JOIN_HPP
