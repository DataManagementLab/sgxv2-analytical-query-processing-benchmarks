#ifndef SGXV2_JOIN_BENCHMARKS_JOINS_HPP
#define SGXV2_JOIN_BENCHMARKS_JOINS_HPP

void
run_join(struct result_t *res, const struct table_t *relR, const struct table_t *relS, const char *algorithm_name,
           const struct joinconfig_t *config);

#endif//SGXV2_JOIN_BENCHMARKS_JOINS_HPP
