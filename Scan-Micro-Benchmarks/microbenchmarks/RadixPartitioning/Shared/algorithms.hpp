#ifndef SGX_ENCRYPTEDDB_ALGORITHMS_HPP
#define SGX_ENCRYPTEDDB_ALGORITHMS_HPP

#include <cstdint>

struct row {
    uint32_t key{0};
    uint32_t value{0};
};

void __attribute__((noinline)) histogram(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift,
                                         uint32_t *hist, uint64_t *hist_cycle_counter);

void __attribute__((noinline)) histogram_unrolled(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift,
                                                  uint32_t *hist, uint64_t *hist_cycle_counter);

void __attribute__((noinline)) histogram_loop(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift,
                                              uint32_t *hist, uint64_t *hist_cycle_counter);

void __attribute__((noinline)) histogram_simd(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift,
                                              uint32_t *hist, uint64_t *hist_cycle_counter);

void __attribute__((noinline)) partition(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift,
                                         uint32_t *sum_hist, row *partitioned, uint64_t *copy_cycle_counter);

void __attribute__((noinline)) partition_unrolled(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift,
                                                  uint32_t *sum_hist, row *partitioned, uint64_t *copy_cycle_counter);

#endif//SGX_ENCRYPTEDDB_ALGORITHMS_HPP
