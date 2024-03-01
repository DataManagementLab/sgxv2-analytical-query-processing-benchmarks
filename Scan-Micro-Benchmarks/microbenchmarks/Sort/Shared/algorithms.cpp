#include <cstdlib>
#include "algorithms.hpp"
#include "rdtscpWrapper.h"

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

void histogram(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift, uint32_t *hist,
               uint64_t *hist_cycle_counter) {
    rdtscpWrapper r{hist_cycle_counter};
    for (uint32_t i = 0; i < data_size; ++i) {
        size_t idx = (data[i].key & mask) >> shift;
        ++hist[idx];
    }
}

void histogram_unrolled(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift, uint32_t *hist,
                        uint64_t *hist_cycle_counter) {
    rdtscpWrapper r{hist_cycle_counter};
    uint32_t i = 0;
    for (; i <= data_size - 8; i += 8) {
        size_t idx = (data[i].key & mask) >> shift;
        size_t idx1 = (data[i + 1].key & mask) >> shift;
        size_t idx2 = (data[i + 2].key & mask) >> shift;
        size_t idx3 = (data[i + 3].key & mask) >> shift;
        size_t idx4 = (data[i + 4].key & mask) >> shift;
        size_t idx5 = (data[i + 5].key & mask) >> shift;
        size_t idx6 = (data[i + 6].key & mask) >> shift;
        size_t idx7 = (data[i + 7].key & mask) >> shift;
        ++hist[idx];
        ++hist[idx1];
        ++hist[idx2];
        ++hist[idx3];
        ++hist[idx4];
        ++hist[idx5];
        ++hist[idx6];
        ++hist[idx7];
    }
    for (; i < data_size; ++i) {
        size_t idx = (data[i].key & mask) >> shift;
        ++hist[idx];
    }
}

void histogram_loop(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift, uint32_t *hist,
                    uint64_t *hist_cycle_counter) {
    rdtscpWrapper r{hist_cycle_counter};
    constexpr size_t unroll_factor = 8;
    auto indexes = (uint32_t *) aligned_alloc(64, unroll_factor * sizeof(uint32_t));
    uint32_t i = 0;
    for (; i <= data_size - unroll_factor; i += unroll_factor) {
        auto current_data = data + i;
        for (uint64_t j = 0; j < unroll_factor; j++) {
            indexes[j] = (current_data[j].key & mask) >> shift;
        }

        for (uint64_t j = 0; j < unroll_factor; j++) {
            ++hist[indexes[j]];
        }
    }
    // handle the remaining entries if data_size is not dividable by unroll_factor
    for (; i < data_size; ++i) {
        size_t idx = (data[i].key & mask) >> shift;
        ++hist[idx];
    }
}

void histogram_simd(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift, uint32_t *hist,
                     uint64_t *hist_cycle_counter) {
    rdtscpWrapper r{hist_cycle_counter};

    auto simd_data = (__m512i *) data;
    auto steps = data_size / 32;

    auto buffer = (uint8_t *) aligned_alloc(64, 128);
    auto int_buffer = (uint32_t *) buffer;

    auto simd_mask = _mm512_set1_epi32(mask);
    auto simd_shift = _mm512_set1_epi32(shift);

    for (uint32_t i = 0; i < steps; ++i) {
        auto vec = _mm512_stream_load_si512(simd_data + (i * 4));
        auto vec2 = _mm512_stream_load_si512(simd_data + (i * 4) + 1);
        auto vec3 = _mm512_stream_load_si512(simd_data + (i * 4) + 2);
        auto vec4 = _mm512_stream_load_si512(simd_data + (i * 4) + 3);
        auto masked = _mm512_and_epi32(vec, simd_mask);
        auto masked2 = _mm512_and_epi32(vec2, simd_mask);
        auto masked3 = _mm512_and_epi32(vec3, simd_mask);
        auto masked4 = _mm512_and_epi32(vec4, simd_mask);
        auto shifted = _mm512_srav_epi32(masked, simd_shift);
        auto shifted2 = _mm512_srav_epi32(masked2, simd_shift);
        auto shifted3 = _mm512_srav_epi32(masked3, simd_shift);
        auto shifted4 = _mm512_srav_epi32(masked4, simd_shift);
        auto compressed = _mm512_cvtepi64_epi32(shifted);
        auto compressed2 = _mm512_cvtepi64_epi32(shifted2);
        auto compressed3 = _mm512_cvtepi64_epi32(shifted3);
        auto compressed4 = _mm512_cvtepi64_epi32(shifted4);
        _mm256_store_epi32(buffer, compressed);
        _mm256_store_epi32(buffer + 32, compressed2);
        _mm256_store_epi32(buffer + 64, compressed3);
        _mm256_store_epi32(buffer + 96, compressed4);

#pragma GCC unroll 32
        for (uint32_t j = 0; j < 32; ++j) {
            ++hist[int_buffer[j]];
        }
    }
    // if data_size % 8 != 0 then handle remaining rows
    for (uint32_t i = steps * 32; i < data_size; ++i) {
        size_t idx = (data[i].key & mask) >> shift;
        ++hist[idx];
    }
}

void partition(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift, uint32_t *sum_hist, row *partitioned,
               uint64_t *copy_cycle_counter) {
    rdtscpWrapper r{copy_cycle_counter};
    for (uint32_t i = 0; i < data_size; ++i) {
        size_t idx = (data[i].key & mask) >> shift;
        partitioned[sum_hist[idx]] = data[i];
        ++sum_hist[idx];
    }
}

void partition_unrolled(const row *data, uint32_t data_size, uint64_t mask, uint64_t shift, uint32_t *sum_hist,
                        row *partitioned, uint64_t *copy_cycle_counter) {
    rdtscpWrapper r{copy_cycle_counter};
    uint32_t i = 0;
    for (; i <= data_size - 4; i += 4) {
        size_t idx = (data[i].key & mask) >> shift;
        size_t idx1 = (data[i + 1].key & mask) >> shift;
        size_t idx2 = (data[i + 2].key & mask) >> shift;
        size_t idx3 = (data[i + 3].key & mask) >> shift;

        partitioned[sum_hist[idx]] = data[i];
        ++sum_hist[idx];
        partitioned[sum_hist[idx1]] = data[i + 1];
        ++sum_hist[idx1];
        partitioned[sum_hist[idx2]] = data[i + 2];
        ++sum_hist[idx2];
        partitioned[sum_hist[idx3]] = data[i + 3];
        ++sum_hist[idx3];
    }
    for (; i < data_size; ++i) {
        size_t idx = (data[i].key & mask) >> shift;
        partitioned[sum_hist[idx]] = data[i];
        ++sum_hist[idx];
    }
}
