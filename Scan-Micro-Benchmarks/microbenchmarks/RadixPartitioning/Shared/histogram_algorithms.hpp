#ifndef HISTOGRAM_ALGORITHMS_HPP
#define HISTOGRAM_ALGORITHMS_HPP

#include "types.hpp"
#include <functional>
#include <optional>

using HistogramFunction = std::function<void(const row *, uint32_t, uint32_t, uint32_t, uint32_t *, uint64_t *)>;

enum class HistogramMode {
    scalar,
    pragma,
    unrolled,
    loop,
    loop_pragma,
    simd_loop,
    simd_loop_unrolled,
    simd_unrolled,
    simd_loop_unrolled_512,
    shuffle
};

struct HistogramParameters {
    const row *data;
    uint32_t data_size;
    uint32_t mask;
    uint32_t shift;
    uint32_t *hist;
};

struct HistogramBenchmarkParameters {
    uint64_t num_keys;
    uint64_t num_bins;
    bool sgx;
    bool preload;
    bool print_header;
    bool ssb_mitigation;
    HistogramMode mode;
    uint32_t unroll_factor;
    uint64_t eid;
    HistogramParameters &function_parameters;
};

std::optional<HistogramFunction>
choose_function(HistogramMode mode, uint32_t unroll_factor);

void __attribute__((noinline))
histogram(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift, uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_pragma(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                 uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_shuffle_2(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                     uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_shuffle_4(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                     uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_shuffle_8(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                     uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_unrolled(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                   uint32_t *hist, uint64_t *cycle_counter);

template<int UNROLL_FACTOR>
void __attribute__((noinline))
histogram_unrolled_max_16(const row *data, uint32_t data_size, uint32_t mask,
                          uint32_t shift, uint32_t *hist, uint64_t *cycle_counter);

template<int UNROLL_FACTOR>
void __attribute__((noinline))
histogram_loop(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
               uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_loop_pragma(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                      uint32_t *hist, uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_simd_loop(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                    uint32_t *hist, uint64_t *cycle_counter);

template<int UNROLL_FACTOR>
void __attribute__((noinline))
histogram_simd_loop_unrolled(const row *data, uint32_t data_size, uint32_t mask,
                             uint32_t shift, uint32_t *hist, uint64_t *cycle_counter);

template<int UNROLL_FACTOR>
void
histogram_simd_loop_unrolled_512(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift, uint32_t *hist,
                                 uint64_t *cycle_counter);

void __attribute__((noinline))
histogram_simd_unrolled(const row *data, uint32_t data_size, uint32_t mask,
                        uint32_t shift, uint32_t *hist, uint64_t *cycle_counter);


#endif //HISTOGRAM_ALGORITHMS_HPP
