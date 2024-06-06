#ifndef Q12PREDICATES_HPP
#define Q12PREDICATES_HPP

#include "TpcHTypes.hpp"
#include "data-types.h"

#include "avx2intrin.h"
#include "avx512bwintrin.h"
#include "avx512cdintrin.h"
#include "avx512dqintrin.h"
#include "avx512fintrin.h"
#include "avx512vbmi2intrin.h"
#include "avx512vbmiintrin.h"
#include "avx512vldqintrin.h"
#include "avx512vlintrin.h"
#include "avxintrin.h"
#include "emmintrin.h"
#include "smmintrin.h"
#include "tmmintrin.h"
#include "xmmintrin.h"

[[gnu::always_inline]] inline bool
q12Predicate(const LineItemTable &l, uint64_t rowID) {
    uint8_t l_shipmode = l.l_shipmode[rowID];
    uint64_t l_commitdate = l.l_commitdate[rowID];
    uint64_t l_shipdate = l.l_shipdate[rowID];
    uint64_t l_receiptdate = l.l_receiptdate[rowID];

    return ((l_shipmode == L_SHIPMODE_MAIL || l_shipmode == L_SHIPMODE_SHIP) && (l_commitdate < l_receiptdate) &&
            (l_shipdate < l_commitdate) && (l_receiptdate >= TIMESTAMP_1994_01_01_SECONDS) &&
            (l_receiptdate < TIMESTAMP_1995_01_01_SECONDS));
}

[[gnu::always_inline]] inline void
q12Copy(const LineItemTable &l, table_t &out, uint64_t from_index, uint64_t to_index) {
    out.tuples[to_index] = l.l_orderkey[from_index];
}

table_t
q12_filter_lineitem(uint64_t num_tuples, const row_t *input_table, const uint8_t *shipmode, const uint64_t *commitdate,
                    const uint64_t *shipdate, const uint64_t *receiptdate) {
    table_t filtered_table{};
    uint64_t selectionMatches = 0;

    filtered_table.tuples = (tuple_t *) aligned_alloc(64, sizeof(tuple_t) * num_tuples);
    malloc_check(filtered_table.tuples);

    // every register contains 64 predicate values
    auto predicate_vectors = num_tuples / 64;
    __m512i shipmode_predicate_1 = _mm512_set1_epi8(L_SHIPMODE_MAIL);
    __m512i shipmode_predicate_2 = _mm512_set1_epi8(L_SHIPMODE_SHIP);

    __m512i receiptdate_predicate_low = _mm512_set1_epi64(TIMESTAMP_1994_01_01_SECONDS);
    __m512i receiptdate_predicate_high = _mm512_set1_epi64(TIMESTAMP_1995_01_01_SECONDS);

    auto shipmode_array_vec = reinterpret_cast<const __m512i *>(shipmode);
    auto commitdate_array_vec = reinterpret_cast<const __m512i *>(commitdate);
    auto shipdate_array_vec = reinterpret_cast<const __m512i *>(shipdate);
    auto receiptdate_array_vec = reinterpret_cast<const __m512i *>(receiptdate);

    auto input_array = reinterpret_cast<const __m512i *>(input_table);
    auto output_buffer = filtered_table.tuples;

    for (uint64_t predicate_vector_i = 0; predicate_vector_i < predicate_vectors; ++predicate_vector_i) {
        __m512i shipmode_vec = _mm512_stream_load_si512((void *) (shipmode_array_vec + predicate_vector_i));
        __mmask64 mask = _mm512_cmpeq_epu8_mask(shipmode_vec, shipmode_predicate_1);
        mask = mask | _mm512_cmpeq_epu8_mask(shipmode_vec, shipmode_predicate_2);

        if (mask == 0) {
            continue;
        }
        // every register contains 64 predicates, but only 8 rows, so we have to iterate over it
        auto pos_64 = (predicate_vector_i * 8);
        for (int j = 0; j < 8; ++j) {
            __mmask8 part_mask_shipmode = (mask >> j * 8) & 0xff;

            __m512i commitdate_vec = _mm512_stream_load_si512((void *) (commitdate_array_vec + pos_64 + j));
            __m512i shipdate_vec = _mm512_stream_load_si512((void *) (shipdate_array_vec + pos_64 + j));
            __m512i receiptdate_vec = _mm512_stream_load_si512((void *) (receiptdate_array_vec + pos_64 + j));

            __mmask8 predicate_2 = _mm512_cmplt_epi64_mask(commitdate_vec, receiptdate_vec);
            __mmask8 predicate_3 = _mm512_cmplt_epi64_mask(shipdate_vec, commitdate_vec);
            __mmask8 predicate_4 = _mm512_cmpge_epi64_mask(receiptdate_vec, receiptdate_predicate_low);
            __mmask8 predicate_5 = _mm512_cmplt_epi64_mask(receiptdate_vec, receiptdate_predicate_high);

            __mmask8 part_mask = part_mask_shipmode & predicate_2 & predicate_3 & predicate_4 & predicate_5;

            if (part_mask == 0) {
                continue;
            }
            _mm512_mask_compressstoreu_epi64(reinterpret_cast<void *>(output_buffer + selectionMatches), part_mask,
                                             _mm512_stream_load_si512((void *) (input_array + pos_64 + j)));
            selectionMatches += __builtin_popcount(part_mask);
        }
    }

    // iterate over the remaining values from the last multiple of 64
    auto rest_start = num_tuples & ~(64 - 1);
    for (uint64_t i = rest_start; i < num_tuples; i++) {
        if ((shipmode[i] == L_SHIPMODE_MAIL || shipmode[i] == L_SHIPMODE_SHIP) && (commitdate[i] < receiptdate[i]) &&
            (shipdate[i] < commitdate[i]) && (receiptdate[i] >= TIMESTAMP_1994_01_01_SECONDS) &&
            (receiptdate[i] < TIMESTAMP_1995_01_01_SECONDS)) {
            filtered_table.tuples[selectionMatches++] = input_table[i];
        }
    }

    filtered_table.num_tuples = selectionMatches;
    return filtered_table;
}

#endif//Q12PREDICATES_HPP
