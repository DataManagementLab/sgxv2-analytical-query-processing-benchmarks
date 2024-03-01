#ifndef JOIN_BENCH_MATTHIAS_NO_PARTITIONING_HASH_JOIN_ST_HPP
#define JOIN_BENCH_MATTHIAS_NO_PARTITIONING_HASH_JOIN_ST_HPP

#include "data-types.h"

result_t*
NPO_st(const table_t *relR, const table_t *relS, const joinconfig_t *config);

result_t *
NPO_no(const table_t *relR, const table_t *relS, const joinconfig_t *config);

#endif //JOIN_BENCH_MATTHIAS_NO_PARTITIONING_HASH_JOIN_ST_HPP
