#ifndef _COMMONS_H_
#define _COMMONS_H_

#include "data-types.h"

struct args_t {
    const algorithm_t *algorithm;
    char algorithm_name[128];
    char r_path[512];
    char s_path[512];
    uint64_t r_size;
    uint64_t s_size;
    uint32_t r_seed;
    uint32_t s_seed;
    uint32_t nthreads;
    uint32_t selectivity;
    double skew;
    int alloc_core;
    int sort_r;
    int sort_s;
    int r_from_path;
    int s_from_path;
    int materialize;
    int mitigation;
};

void parse_args(int argc, char **argv, args_t *params, const struct algorithm_t algorithms[]);

void print_relation(relation_t *rel, uint32_t num, uint32_t offset);

#endif // _COMMONS_H_
