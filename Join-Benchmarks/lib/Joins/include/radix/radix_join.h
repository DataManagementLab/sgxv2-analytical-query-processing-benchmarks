#ifndef _RADIX_JOIN_H_
#define _RADIX_JOIN_H_

#include <stdlib.h>
#include "data-types.h"

using JoinFunction = int64_t (*)(const struct table_t *const,
                                 const struct table_t *const,
                                 struct table_t *const,
                                 uint32_t,
                                 #ifdef CHUNKED_TABLE
                                 chunked_table_t *output,
                                 #else
                                 output_list_t **output,
                                 #endif
                                 uint64_t *build_timer,
                                 uint64_t *join_timer,
                                 int materialize);

void print_timing(uint64_t join_runtime, uint64_t join, uint64_t partition,
                  uint64_t num_tuples, int64_t result,
                  uint64_t start, uint64_t end,
                  uint64_t pass1, uint64_t pass2,
                  uint64_t build_in_depth, uint64_t join_in_depth);

result_t *
RJ(const table_t *relR, const table_t *relS, const joinconfig_t *config);

result_t *
RHO(const table_t *relR, const table_t *relS, const joinconfig_t *config);

result_t *
RHT(const table_t *relR, const table_t *relS, const joinconfig_t *config);

result_t *
join_init_run(const table_t *relR, const table_t *relS, JoinFunction jf, const joinconfig_t *config);

#endif  //_RADIX_JOIN_H_