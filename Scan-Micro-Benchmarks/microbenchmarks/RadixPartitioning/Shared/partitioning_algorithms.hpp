#ifndef SGX_ENCRYPTEDDB_ALGORITHMS_HPP
#define SGX_ENCRYPTEDDB_ALGORITHMS_HPP

#include "types.hpp"

#include <functional>
#include <optional>

using PartitioningFunction =
        std::function<void(const row *, uint32_t, uint32_t, uint32_t, uint32_t *, row *, uint64_t *)>;

enum class PartitioningMode { scalar, unrolled, loop };

std::optional<PartitioningFunction>
choose_function(PartitioningMode mode, uint32_t unroll_factor);

void __attribute__((noinline)) partition(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                                         uint32_t *sum_hist, row *partitioned, uint64_t *copy_cycle_counter);

template<int UNROLL_FACTOR>
void __attribute__((noinline))
partition_unrolled(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                   uint32_t *sum_hist, row *partitioned, uint64_t *copy_cycle_counter);

template<int UNROLL_FACTOR>
void __attribute__((noinline))
partition_loop(const row *data, uint32_t data_size, uint32_t mask, uint32_t shift,
                   uint32_t *sum_hist, row *partitioned, uint64_t *copy_cycle_counter);

#endif//SGX_ENCRYPTEDDB_ALGORITHMS_HPP