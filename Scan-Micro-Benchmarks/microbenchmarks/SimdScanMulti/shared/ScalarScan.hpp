#ifndef SGX_ENCRYPTEDDB_SCALARSCAN_HPP
#define SGX_ENCRYPTEDDB_SCALARSCAN_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

void scalar_implicit_index_scan(uint8_t predicate_low, uint8_t predicate_high,
                                const uint8_t *input_compressed,
                                size_t input_size,
                                CacheAlignedVector<size_t> &output_buffer,
                                bool cut = false) {
    size_t written_outputs = 0;
    for (size_t i = 0; i < input_size; ++i) {
        if (input_compressed[i] >= predicate_low && input_compressed[i] <= predicate_high) {
            output_buffer[written_outputs] = i;
            ++written_outputs;
        }
    }
}

#endif//SGX_ENCRYPTEDDB_SCALARSCAN_HPP
