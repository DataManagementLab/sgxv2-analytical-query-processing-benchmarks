#include "Enclave_t.h" // structs defined in .edl file etc

#include <vector>

#include "rdtscpWrapper.h"
#include "Allocator.hpp"
#include "Barrier.hpp"
#include "../Shared/algorithms.hpp"

constexpr int CACHE_LINE_SIZE = 64;

row *preload_data = nullptr;
row *preload_partitioned = nullptr;
uint32_t *preload_hist = nullptr;
uint32_t *preload_sum_hist = nullptr;

extern "C" {

void ecall_noop() {
    //for benchmarking
}

void ecall_init_data(size_t data_size, uint64_t radix_bits, uint32_t padding) {
    auto fan_out = 1 << radix_bits;
    try {
        preload_data = (row *) aligned_alloc(CACHE_LINE_SIZE, data_size * sizeof(row));
        preload_partitioned = (row *) aligned_alloc(CACHE_LINE_SIZE, sizeof(row) * (data_size + fan_out * padding));
        memset(preload_partitioned, 42, sizeof(row) * (data_size + fan_out * padding));
    } catch (const std::bad_alloc &error) {
        ocall_print_error("Allocation inside enclave failed with std::bad_alloc!");
        throw error;
    }
}

void ecall_next_num_keys(size_t data_size, uint32_t num_keys, uint64_t radix_bits) {
    auto fan_out = 1 << radix_bits;
    for (uint32_t i = 0; i < data_size; ++i) {
        preload_data[i].key = i % num_keys;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(preload_data, preload_data + data_size, g);

    preload_hist = (uint32_t *) aligned_alloc(CACHE_LINE_SIZE, fan_out * sizeof(uint32_t));
    std::memset(preload_hist, 0, fan_out * 4);
    preload_sum_hist = (uint32_t *) aligned_alloc(CACHE_LINE_SIZE, fan_out * sizeof(uint32_t));
    std::memset(preload_sum_hist, 0, fan_out * 4);
}

void ecall_histogram(size_t data_size, uint64_t mask, uint64_t shift, uint64_t *hist_cycle_counter) {
    histogram(preload_data, data_size, mask,  shift, preload_hist, hist_cycle_counter);
}

void ecall_histogram_unrolled(size_t data_size, uint64_t mask, uint64_t shift,
                                   uint64_t *hist_cycle_counter) {
    histogram_unrolled(preload_data, data_size, mask, shift, preload_hist, hist_cycle_counter);
}

void ecall_histogram_loop(size_t data_size, uint64_t mask, uint64_t shift,
                                   uint64_t *hist_cycle_counter) {
    histogram_loop(preload_data, data_size, mask, shift, preload_hist, hist_cycle_counter);
}

void ecall_histogram_user_check(void * o_data, size_t data_size, void * o_hist, uint64_t mask,
                                     uint64_t shift, uint64_t * hist_cycle_counter) {
    auto outside_data = reinterpret_cast<row *>(o_data);
    auto outside_hist = reinterpret_cast<uint32_t *>(o_hist);
    histogram(outside_data, data_size, mask, shift, outside_hist, hist_cycle_counter);
}

void ecall_histogram_user_check_unrolled(void * o_data, size_t data_size, void * o_hist, uint64_t mask,
                                uint64_t shift, uint64_t * hist_cycle_counter) {
    auto outside_data = reinterpret_cast<row *>(o_data);
    auto outside_hist = reinterpret_cast<uint32_t *>(o_hist);
    histogram_unrolled(outside_data, data_size, mask, shift, outside_hist, hist_cycle_counter);
}

void ecall_histogram_simd(size_t data_size, uint64_t mask, uint64_t shift,
                          uint64_t *hist_cycle_counter) {
    histogram_simd(preload_data, data_size, mask, shift, preload_hist, hist_cycle_counter);
}

void ecall_cumsum_hist(uint64_t radix_bits, uint32_t padding) {
    auto fan_out = 1 << radix_bits;
    std::partial_sum(preload_hist, preload_hist + fan_out - 1, preload_sum_hist + 1);

    for (uint32_t i = 0; i < fan_out; i++) {
        preload_sum_hist[i] += i * padding;
    }
}

void ecall_partition(size_t data_size, uint64_t mask, uint64_t shift, uint64_t *copy_cycle_counter) {
    partition(preload_data, data_size, mask, shift, preload_sum_hist, preload_partitioned, copy_cycle_counter);
}

void ecall_partition_user_check(void * o_data, size_t data_size, uint32_t * outside_sum_hist, void * o_partitioned,
                                uint64_t mask, uint64_t shift, uint64_t *copy_cycle_counter) {
    auto outside_data = reinterpret_cast<row *>(o_data);
    auto outside_partitioned = reinterpret_cast<row *>(o_partitioned);

    partition(outside_data, data_size, mask, shift, outside_sum_hist, outside_partitioned, copy_cycle_counter);
}

void ecall_partition_unrolled(size_t data_size, uint64_t mask, uint64_t shift, uint64_t *copy_cycle_counter) {
    partition_unrolled(preload_data, data_size, mask, shift, preload_sum_hist, preload_partitioned, copy_cycle_counter);
}

void ecall_partition_user_check_unrolled(void * o_data, size_t data_size, uint32_t * outside_sum_hist,
                                         void * o_partitioned, uint64_t mask, uint64_t shift,
                                         uint64_t *copy_cycle_counter) {
    auto outside_data = reinterpret_cast<row *>(o_data);
    auto outside_partitioned = reinterpret_cast<row *>(o_partitioned);

    partition_unrolled(outside_data, data_size, mask, shift, outside_sum_hist, outside_partitioned, copy_cycle_counter);
}

void ecall_run_benchmark(size_t data_size, uint64_t radix_bits, uint64_t shift, uint32_t padding,
                         uint64_t *hist_cycle_counter, uint64_t *copy_cycle_counter) {
    uint64_t mask = (1 << radix_bits) - 1;
    auto fan_out = 1 << radix_bits;

    {
        rdtscpWrapper r{hist_cycle_counter};
        for (uint32_t i = 0; i < data_size; ++i) {
            size_t idx = (preload_data[i].key & mask) >> shift;
            ++preload_hist[idx];
        }
    }

    std::partial_sum(preload_hist, preload_hist + fan_out - 1, preload_sum_hist + 1);
    for (uint32_t i = 0; i < fan_out; i++) {
        preload_sum_hist[i] += i * padding;
    }

    {
        rdtscpWrapper r{copy_cycle_counter};
        for (uint32_t i = 0; i < data_size; ++i) {
            size_t idx = (preload_data[i].key & mask) >> shift;
            preload_partitioned[preload_sum_hist[idx]] = preload_data[i];
            ++preload_sum_hist[idx];
        }
    }
}

void ecall_free_preload_hist() {
    free(preload_hist);
    free(preload_sum_hist);
    preload_hist = nullptr;
    preload_sum_hist = nullptr;
}

void ecall_free_preload_data() {
    free(preload_data);
    free(preload_partitioned);
    preload_data = nullptr;
    preload_partitioned = nullptr;
}


}