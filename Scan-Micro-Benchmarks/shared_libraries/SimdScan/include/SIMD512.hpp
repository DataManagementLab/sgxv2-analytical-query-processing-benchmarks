#pragma once

#ifdef ENCLAVE
#include "../simd-headers/emmintrin.h"
#include "../simd-headers/smmintrin.h"
#include "../simd-headers/tmmintrin.h"
#include "../simd-headers/xmmintrin.h"
#include "../simd-headers/avxintrin.h"
#include "../simd-headers/avx2intrin.h"
#include "../simd-headers/avx512fintrin.h"
#include "../simd-headers/avx512cdintrin.h"
#include "../simd-headers/avx512vlintrin.h"
#include "../simd-headers/avx512bwintrin.h"
#include "../simd-headers/avx512cdintrin.h"
#include "../simd-headers/avx512fintrin.h"
#include "../simd-headers/avx512vbmiintrin.h"
#include "../simd-headers/avx512vbmi2intrin.h"
#else
#include <immintrin.h>
#endif

#include <cstdint>
#include <array>
#include <Allocator.hpp>

template<typename T>
using CacheAlignedVector = std::vector<T, AlignedAllocator<T>>;

namespace SIMD512 {

    constexpr uint8_t BITS_NEEDED = 8;
    using pred_t = uint8_t;

    /**
     * SIMD COUNT - Counts compressed input for a range of predicates: (predicate_low<=key<=predicate_high)
     * Input: low & high predicate, compressed bitstream, input size (#compressed integers)
     * Return: number of tuples found in that range
     */
    size_t count(pred_t predicate_low, pred_t predicate_high, const __m512i *input_compressed, size_t input_size);

    /**
     * SIMD SUM - Sums compressed input for a range of predicates: (predicate_low<=key<=predicate_high)
     * Input: low & high predicate, compressed bitstream, input size (#compressed integers)
     * Return: sum of tuples found in that range
     */
    size_t sum(pred_t predicate_low, pred_t predicate_high, const __m512i *input_compressed, size_t input_size);

    /**
     * SIMD SCAN - Scans compressed input for a range of predicates: (predicate_low<=key<=predicate_high)
     * Input: low & high predicate, compressed bitstream, input size (#compressed integers)
     * Output: uncompressed and filtered array of integers
     * Return: number of tuples found in that range
     */
    size_t scan(pred_t predicate_low,
                pred_t predicate_high,
                const __m512i *input_compressed,
                size_t input_size,
                uint32_t *output_buffer);

    void explicit_index_scan(pred_t predicate_low,
                             pred_t predicate_high,
                             const __m512i *index_compressed,
                             const __m512i *input_compressed,
                             size_t input_size,
                             size_t *output_buffer);

    void implicit_index_scan(pred_t predicate_low,
                             pred_t predicate_high,
                             const __m512i *input_compressed,
                             size_t input_size,
                             size_t *output_buffer);

    void implicit_index_scan_self_alloc(pred_t predicate_low,
                                        pred_t predicate_high,
                                        const __m512i *input_compressed,
                                        size_t input_size,
                                        CacheAlignedVector<size_t> &output_buffer,
                                        bool cut = false);

    void bitvector_scan(pred_t predicate_low,
                        pred_t predicate_high,
                        const __m512i *input_compressed,
                        size_t input_size,
                        __mmask64 *output_buffer);

    void
    bitvector_scan_16bit(uint16_t predicate_low, uint16_t predicate_high, const __m512i *input_compressed,
                   size_t input_size, __mmask32 *output_buffer);

    void
    dict_scan_8bit_64bit(int64_t predicate_low,
                         int64_t predicate_high,
                         const int64_t *dict,
                         const __m512i * input_compressed,
                         size_t input_size,
                         CacheAlignedVector<int64_t> &output_buffer,
                         bool cut = false);

    void dict_scan_8bit_64bit_scalar_gather_scatter(int64_t predicate_low,
                                                    int64_t predicate_high,
                                                    const int64_t *dict,
                                                    const __m512i *input_compressed,
                                                    size_t input_size,
                                                    CacheAlignedVector<int64_t> &output_buffer,
                                                    bool cut = false);

    void dict_scan_8bit_64bit_scalar_unroll(int64_t predicate_low, int64_t predicate_high, const int64_t *dict,
                                            const __m512i *input_compressed, size_t input_size,
                                            CacheAlignedVector<int64_t> &output_buffer, bool cut);

    void dict_scan_8bit_64bit_opt_write(int64_t predicate_low, int64_t predicate_high, const int64_t *dict,
                                        const __m512i *input_compressed, size_t input_size,
                                        CacheAlignedVector<int64_t> &output_buffer, bool cut);

    void dict_scan_8bit_64bit_scalar(int64_t predicate_low, int64_t predicate_high, const int64_t *dict,
                                     const uint8_t *input_compressed, size_t input_size,
                                     CacheAlignedVector<int64_t> &output_buffer);

    void
    dict_scan_16bit_64bit(int64_t predicate_low,
                          int64_t predicate_high,
                          const int64_t *dict,
                          const __m512i *input_compressed,
                          size_t input_size,
                          CacheAlignedVector<int64_t> &output_buffer);

    void
    dict_scan_32bit_64bit(int64_t predicate_low,
                          int64_t predicate_high,
                          const int64_t *dict,
                          size_t dict_size,
                          const __m512i *input_compressed,
                          size_t input_size,
                          CacheAlignedVector<int64_t> &output_buffer);

    void dict_scan_32bit_64bit_scalar_gather_scatter(int64_t predicate_low,
                                                     int64_t predicate_high,
                                                     const int64_t *dict,
                                                     size_t dict_size,
                                                     const __m512i *input_compressed,
                                                     size_t input_size,
                                                     CacheAlignedVector<int64_t> &output_buffer,
                                                     bool cut = false);

} // Namespace SIMD512