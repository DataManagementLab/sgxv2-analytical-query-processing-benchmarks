#ifndef SGXV2_JOIN_BENCHMARKS_PRINTER_HPP
#define SGXV2_JOIN_BENCHMARKS_PRINTER_HPP

#include <cstdint>

struct TPCHTimers {
    uint64_t selection_1;
    uint64_t selection_2;
    uint64_t selection_3;
    uint64_t join_1;
    uint64_t join_2;
    uint64_t join_3;
    uint64_t copy;
    uint64_t total;
};

TPCHTimers
timers_to_us(const TPCHTimers &src);

void
print_query_results(const TPCHTimers &timers, uint64_t numTuples);

#endif//SGXV2_JOIN_BENCHMARKS_PRINTER_HPP
