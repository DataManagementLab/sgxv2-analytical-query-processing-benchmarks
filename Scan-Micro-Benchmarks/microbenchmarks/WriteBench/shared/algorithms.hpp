#ifndef SGX_ENCRYPTEDDB_ALGORITHMS_HPP
#define SGX_ENCRYPTEDDB_ALGORITHMS_HPP

#include <cstdint>

constexpr uint64_t DEFAULT_REPETITIONS = 1'000'000'000;

template<uint64_t REPETITIONS = DEFAULT_REPETITIONS>
void __attribute__((noinline))
random_write(uint64_t *data, uint64_t mask, uint64_t *cycle_counter);

template<uint64_t REPETITIONS = DEFAULT_REPETITIONS>
void __attribute__((noinline))
random_write_unrolled(uint64_t *data, uint64_t mask, uint64_t *cycle_counter);

/**
 * Writes an aligned region of data with size write_size_byte with a random value. This happens repetitions times.
 * Make sure that mask only returns addresses divisible by size.
 * @param data The memory region to write to
 * @param mask The mask to use. Make sure that the mask value is not larger than the memory region - write_size_byte
 * @param write_size_byte How many bytes to write per operation
 * @param repetitions How often to create a random position and write to it
 * @param cycle_counter [out] Time taken in CPU cycles
 */
void __attribute__((noinline))
random_write_size(uint8_t *data, uint64_t mask, uint64_t write_size_byte, uint64_t repetitions,
                  uint64_t *cycle_counter);

template<uint64_t REPETITIONS = DEFAULT_REPETITIONS>
void __attribute__((noinline))
random_increment(uint64_t *data, uint64_t mask, uint64_t *cycle_counter);

template<uint64_t REPETITIONS = DEFAULT_REPETITIONS>
void __attribute__((noinline))
random_read(uint64_t *data, uint64_t mask, uint64_t *cycle_counter);

template<uint64_t REPETITIONS = DEFAULT_REPETITIONS>
void __attribute__((noinline))
non_temporal_write(uint8_t *data, uint64_t data_size, uint64_t *cycle_counter);

#endif //SGX_ENCRYPTEDDB_ALGORITHMS_HPP
