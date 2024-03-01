#ifndef SGX_ENCRYPTEDDB_RESULTALLOCATORS_HPP
#define SGX_ENCRYPTEDDB_RESULTALLOCATORS_HPP

#include "SIMD512.hpp"
#include <vector>

template <class T>
void pre_alloc_per_thread(const uint8_t * data, std::vector<CacheAlignedVector<T>> &result_vectors, size_t num_threads,
                          uint8_t predicate_low, uint8_t predicate_high, size_t num_records_per_thread) {
    result_vectors.resize(num_threads);
    for (auto i = 0; i < num_threads; ++i) {
        result_vectors[i].clear();
        auto count = SIMD512::count(predicate_low,
                                    predicate_high,
                                    (__m512i *) (data + i * num_records_per_thread),
                                    num_records_per_thread);
        result_vectors[i].resize(count + 64); // Adding 64 makes sure that the algorithm does not increase the vector size
    }
}

#endif //SGX_ENCRYPTEDDB_RESULTALLOCATORS_HPP
