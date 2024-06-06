#ifndef SIMPLE_ALGORITHMS_HPP
#define SIMPLE_ALGORITHMS_HPP

#include "RNG.hpp"


#include <cstdint>
#include <functional>
#include <optional>
#include <variant>

using SimpleFunction64 = std::function<void(const uint64_t *, uint32_t, uint32_t *, uint64_t *)>;
using SimpleFunction32 = std::function<void(const uint32_t *, uint32_t, uint32_t *, uint64_t *)>;

enum class SimpleMode {
    scalar,
    unrolled,
    loop,
    loop_pointer,
    loop_triple,
    loop_const,
    ptr_loop,
    ptr_scalar,
};

struct SimpleParameters {
    const uint64_t *data;
    uint32_t data_size;
    uint32_t *out;
};

struct SimpleBenchmarkParameters {
    uint64_t num_bins;
    bool sgx;
    bool preload;
    bool print_header;
    bool ssb_mitigation;
    bool d64;
    SimpleMode mode;
    RandomMode random_mode;
    uint32_t unroll_factor;
    bool cache_preheat;
    uint64_t eid;
    SimpleParameters &function_parameters;
};

template<bool USE_64_BIT=false>
std::optional<std::conditional_t<USE_64_BIT, SimpleFunction64, SimpleFunction32>>
choose_function_simple(SimpleMode mode, uint32_t unroll_factor);

template<typename T>
void __attribute__((noinline))
scalar(const T *data, uint32_t data_size, uint32_t *out, uint64_t *cycle_counter);

template<typename T>
void __attribute__((noinline))
unrolled(const T *data, uint32_t data_size, uint32_t *out, uint64_t *cycle_counter);

template<typename T, int UNROLL_FACTOR>
void __attribute__((noinline))
simple_loop(const T *data, uint32_t data_size, uint32_t *out, uint64_t *cycle_counter);

template<typename T, int UNROLL_FACTOR>
void __attribute__((noinline))
simple_const_loop(const T *data, uint32_t data_size, uint32_t *out, uint64_t *cycle_counter);

void __attribute__((noinline))
simple_pointers_scalar(const uint64_t *data, const uint32_t data_size, uint32_t *out, uint64_t *cycle_counter);

template<int UNROLL_FACTOR>
void __attribute__((noinline))
simple_pointers_loop(const uint64_t *data, uint32_t data_size, uint32_t *out, uint64_t *cycle_counter);

#endif //SIMPLE_ALGORITHMS_HPP
