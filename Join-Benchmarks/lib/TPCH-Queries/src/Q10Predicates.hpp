#ifndef Q10PREDICATES_HPP
#define Q10PREDICATES_HPP

#include "TpcHTypes.hpp"
#include "data-types.h"
#include "util.hpp"
#include <cstdlib>

#include "emmintrin.h"
#include "smmintrin.h"
#include "tmmintrin.h"
#include "xmmintrin.h"
#include "avxintrin.h"
#include "avx2intrin.h"
#include "avx512fintrin.h"
#include "avx512cdintrin.h"
#include "avx512vlintrin.h"
#include "avx512bwintrin.h"
#include "avx512cdintrin.h"
#include "avx512fintrin.h"
#include "avx512vbmiintrin.h"
#include "avx512vbmi2intrin.h"
#include "avx512dqintrin.h"
#include "avx512vldqintrin.h"

[[gnu::always_inline]] inline bool
q10OrdersPredicate(const OrdersTable &o, uint64_t rowID) {
    return o.o_orderdate[rowID] >= TIMESTAMP_1993_10_01_SECONDS && o.o_orderdate[rowID] < TIMESTAMP_1994_01_01_SECONDS;
}

[[gnu::always_inline]] inline void
q10OrderCopy(const OrdersTable &o, table_t &out, uint64_t from_index, uint64_t to_index) {
    out.tuples[to_index].key = o.o_custkey[from_index];
    out.tuples[to_index].payload = o.o_orderkey[from_index].payload;
}

[[gnu::always_inline]] inline bool
q10LineItemPredicate(const LineItemTable &l, uint64_t rowID) {
    return l.l_returnflag[rowID] == L_RETURNFLAG_R;
}

[[gnu::always_inline]] inline void
q10LineItemCopy(const LineItemTable &l, table_t &out, uint64_t from_index, uint64_t to_index) {
    out.tuples[to_index] = l.l_orderkey[from_index];
}

table_t
q10_filter_order_simd(uint64_t num_tuples, const type_key *custkey, const row_t *orderkey, const uint64_t *orderdate) {
    table_t filtered_table{};
    uint64_t selectionMatches = 0;

    filtered_table.tuples = (tuple_t *) aligned_alloc(64, sizeof(tuple_t) * num_tuples);
    malloc_check(filtered_table.tuples);

    // every register contains 8 predicate values
    auto predicate_vectors = num_tuples / 8;
    __m512i predicate_low = _mm512_set1_epi64(TIMESTAMP_1993_10_01_SECONDS);
    __m512i predicate_high = _mm512_set1_epi64(TIMESTAMP_1994_01_01_SECONDS);
    auto predicate_array_vec = reinterpret_cast<const __m512i *>(orderdate);

    for (uint64_t predicate_vector_i = 0; predicate_vector_i < predicate_vectors; ++predicate_vector_i) {
        __mmask8 mask_low = _mm512_cmpge_epu64_mask(predicate_array_vec[predicate_vector_i], predicate_low);
        __mmask8 mask_high = _mm512_cmplt_epu64_mask(predicate_array_vec[predicate_vector_i], predicate_high);
        __mmask8 mask = mask_high & mask_low;
        if (mask == 0) {
            continue;
        }

        // I cannot be bothered to build table rows with SIMD right now.
        // Although it would work by interleaved merging of two 256 bit vectors. It is unclear if the unaligned
        // compress store used above even is better.
        for (uint64_t bit = 0; bit < 8; ++bit, mask >>= 1) {
            if (mask & 1) {
                filtered_table.tuples[selectionMatches].key = custkey[predicate_vector_i * 8 + bit];
                filtered_table.tuples[selectionMatches].payload = orderkey[predicate_vector_i * 8 + bit].payload;
                selectionMatches++;
            }
        }
    }

    auto rest_start = num_tuples & ~(8 - 1);
    for (uint64_t i = rest_start; i < num_tuples; i++) {
        if (orderdate[i] < TIMESTAMP_1995_03_15_SECONDS) {
            filtered_table.tuples[selectionMatches].key = custkey[i];
            filtered_table.tuples[selectionMatches].payload = orderkey[i].payload;
            selectionMatches++;
        }
    }

    filtered_table.num_tuples = selectionMatches;
    return filtered_table;
}

table_t
q10_filter_lineitem_simd(uint64_t num_tuples, const row_t *orderkey, const char *return_flag) {
    // TODO evaluate if filter using SIMD is faster
    table_t filtered_table{};
    uint64_t selectionMatches = 0;

    filtered_table.tuples = (tuple_t *) aligned_alloc(64, sizeof(tuple_t) * num_tuples);
    malloc_check(filtered_table.tuples);

    // every register contains 64 predicate values
    auto predicate_vectors = num_tuples / 64;
    __m512i predicate = _mm512_set1_epi8(L_RETURNFLAG_R);

    auto predicate_array_vec = reinterpret_cast<const __m512i *>(return_flag);
    auto input_array = reinterpret_cast<const __m512i *>(orderkey);
    auto output_buffer = filtered_table.tuples;

    for (uint64_t predicate_vector_i = 0; predicate_vector_i < predicate_vectors; ++predicate_vector_i) {
        __mmask64 mask = _mm512_cmpeq_epu8_mask(predicate_array_vec[predicate_vector_i], predicate);
        if (mask == 0) {
            continue;
        }
        // every register contains 64 predicates, but only 8 rows, so we have to iterate over it
        auto pos_input = input_array + (predicate_vector_i * 8);
        // TODO alternatively do the copy iteratively
        for (int j = 0; j < 8; ++j) {
            __mmask8 part_mask = (mask >> j * 8) & 0xff;
            if (part_mask == 0) {
                continue;
            }
            _mm512_mask_compressstoreu_epi64(reinterpret_cast<void *>(output_buffer + selectionMatches),
                                             part_mask,
                                             *(pos_input + j));
            selectionMatches += __builtin_popcount(part_mask);
        }
    }

    // iterate over the remaining values from the last multiple of 64
    auto rest_start = num_tuples & ~(64 - 1);
    for (uint64_t i = rest_start; i < num_tuples; i++) {
        if (return_flag[i] == MKT_BUILDING) {
            filtered_table.tuples[selectionMatches++] = orderkey[i];
        }
    }

    filtered_table.num_tuples = selectionMatches;
    return filtered_table;
}

#endif//Q10PREDICATES_HPP
