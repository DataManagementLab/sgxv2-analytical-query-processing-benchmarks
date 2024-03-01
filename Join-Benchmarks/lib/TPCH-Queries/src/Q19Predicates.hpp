#ifndef Q19PREDICATES_HPP
#define Q19PREDICATES_HPP

#include <pthread.h>
#include "TpcHTypes.hpp"
#include "data-types.h"
#include "Logger.hpp"

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

#include <atomic>
#include <array>

[[gnu::always_inline, nodiscard]] inline bool
q19LineItemPredicate(const LineItemTable &l, uint64_t rowID) {
    return (l.l_quantity[rowID] >= 1 && l.l_quantity[rowID] <= (20 + 10)) &&
           (l.l_shipmode[rowID] == L_SHIPMODE_AIR || l.l_shipmode[rowID] == L_SHIPMODE_AIR_REG) &&
           (l.l_shipinstruct[rowID] == L_SHIPINSTRUCT_DELIVER_IN_PERSON);
}

[[gnu::always_inline]] inline void
q19LineItemCopy(const LineItemTable &l, table_t &out, uint64_t from_index, uint64_t to_index) {
    out.tuples[to_index].key = l.l_partkey[from_index];
    out.tuples[to_index].payload = l.l_orderkey[from_index].payload;
}

[[gnu::always_inline, nodiscard]] inline bool
q19PartPredicate(const PartTable &p, uint64_t rowID) {
    return (p.p_brand[rowID] == P_BRAND_12 || p.p_brand[rowID] == P_BRAND_23 || p.p_brand[rowID] == P_BRAND_34) &&
           (p.p_container[rowID] == P_CONTAINER_SM_CASE || p.p_container[rowID] == P_CONTAINER_SM_BOX ||
            p.p_container[rowID] == P_CONTAINER_SM_PACK || p.p_container[rowID] == P_CONTAINER_SM_PKG ||
            p.p_container[rowID] == P_CONTAINER_MED_BAG || p.p_container[rowID] == P_CONTAINER_MED_BOX ||
            p.p_container[rowID] == P_CONTAINER_MED_PKG || p.p_container[rowID] == P_CONTAINER_MED_PACK ||
            p.p_container[rowID] == P_CONTAINER_LG_CASE || p.p_container[rowID] == P_CONTAINER_LG_BOX ||
            p.p_container[rowID] == P_CONTAINER_LG_PACK || p.p_container[rowID] == P_CONTAINER_LG_PKG) &&
           (p.p_size[rowID] >= 1 && p.p_size[rowID] <= 15);
}

[[gnu::always_inline]] inline void
q19PartCopy(const PartTable &p, table_t &out, uint64_t from_index, uint64_t to_index) {
    out.tuples[to_index] = p.p_partkey[from_index];
}

[[gnu::always_inline, nodiscard]] inline bool
q19FinalPredicate(const PartTable *p, uint64_t rowIdPart, const LineItemTable *l, uint64_t rowIdLineItem) {
    bool p1 = p->p_brand[rowIdPart] == P_BRAND_12 &&
              (p->p_container[rowIdPart] == P_CONTAINER_SM_CASE || p->p_container[rowIdPart] == P_CONTAINER_SM_BOX ||
               p->p_container[rowIdPart] == P_CONTAINER_SM_PACK || p->p_container[rowIdPart] == P_CONTAINER_SM_PKG) &&
              (p->p_size[rowIdPart] >= 1 && p->p_size[rowIdPart] <= 5) &&
              (l->l_quantity[rowIdLineItem] >= 1 && l->l_quantity[rowIdLineItem] <= (1 + 10));

    bool p2 = p->p_brand[rowIdPart] == P_BRAND_23 &&
              (p->p_container[rowIdPart] == P_CONTAINER_MED_BAG || p->p_container[rowIdPart] == P_CONTAINER_MED_BOX ||
               p->p_container[rowIdPart] == P_CONTAINER_MED_PKG || p->p_container[rowIdPart] == P_CONTAINER_MED_PACK) &&
              (p->p_size[rowIdPart] >= 1 && p->p_size[rowIdPart] <= 10) &&
              (l->l_quantity[rowIdLineItem] >= 10 && l->l_quantity[rowIdLineItem] <= (10 + 10));

    bool p3 = p->p_brand[rowIdPart] == P_BRAND_34 &&
              (p->p_container[rowIdPart] == P_CONTAINER_LG_CASE || p->p_container[rowIdPart] == P_CONTAINER_LG_BOX ||
               p->p_container[rowIdPart] == P_CONTAINER_LG_PACK || p->p_container[rowIdPart] == P_CONTAINER_LG_PKG) &&
              (p->p_size[rowIdPart] >= 1 && p->p_size[rowIdPart] <= 15) &&
              (l->l_quantity[rowIdLineItem] >= 20 && l->l_quantity[rowIdLineItem] <= (20 + 10));

    return p1 || p2 || p3;
}

struct Q19ThreadArg {
    uint64_t matches;
    output_list_t *threadresult;
    const LineItemTable *l;
    const PartTable *p;
    uint64_t nresults;
};

void *
q19FilterThread(void *param) {
    auto *arg = static_cast<Q19ThreadArg *>(param);
    output_list_t *head = arg->threadresult;
    while (head != nullptr) {
        type_value rowIdPart = head->Rpayload;
        type_value rowIdLineItem = head->Spayload;
        if (q19FinalPredicate(arg->p, rowIdPart, arg->l, rowIdLineItem)) {
            arg->matches++;
        }
        head = head->next;
    }
    return nullptr;
}

uint64_t
q19FilterJoinResults(result_t *jr, const LineItemTable *l, const PartTable *p) {
    int nthreads = jr->nthreads;
    pthread_t tid[nthreads];
    auto *args = new Q19ThreadArg[nthreads];

    int rv = 0;
    for (int i = 0; i < nthreads; i++) {
        auto thread_result = ((threadresult_t *) jr->result)[i];
        args[i].threadresult = thread_result.results;
        args[i].nresults = thread_result.nresults;
        args[i].p = p;
        args[i].l = l;
        args[i].matches = 0;
        rv = pthread_create(&tid[i], nullptr, q19FilterThread, (void *) &args[i]);
        if (rv) {
            logger(ERROR, "return code from pthread_create() is %d\n", rv);
        }
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tid[i], nullptr);
    }
    uint64_t totalMatches = 0;
    for (int i = 0; i < nthreads; i++) {
        totalMatches += args[i].matches;
    }

    delete[] args;
    return totalMatches;
}

struct Q19ChunkedThreadArg {
    uint64_t matches;
    uint64_t thread_id;
    chunked_table_t *input_table;
    std::atomic<uint64_t> *next_chunk;
    const LineItemTable *l;
    const PartTable *p;
};

void *
q19FilterChunkedThread(void *args) {
    auto parameters = static_cast<Q19ChunkedThreadArg *>(args);

    // Each threads starts scanning the chunk with its own ID. Since there are at least as many chunks as threads,
    // this always works.
    uint64_t chunk_id = parameters->thread_id;
    // Take the last chunk ID as long as there are chunks left.
    do {
        auto chunk = parameters->input_table->chunks[chunk_id];

        // Iterate over all tuples in the chunk
        for (uint64_t tuple_id = 0; tuple_id < chunk->num_tuples; ++tuple_id) {
            auto tuple = chunk->tuples[tuple_id];
            parameters->matches += static_cast<uint8_t>(q19FinalPredicate(parameters->p, tuple.Rpayload, parameters->l,
                                                                          tuple.Spayload));
        }
        // Atomically get the next chunk id and increment, preventing two threads from getting the same chunk ID.
    } while ((chunk_id = parameters->next_chunk->fetch_add(1)) < parameters->input_table->num_chunks);

    return nullptr;
}

uint64_t
q19FilterJoinResultsChunked(const result_t *join_result, const LineItemTable *l, const PartTable *p) {
    uint64_t num_threads = join_result->nthreads;
    std::vector<pthread_t> thread_ids(num_threads);
    std::vector<Q19ChunkedThreadArg> args(num_threads);

    std::atomic next_chunk{num_threads};

    for (uint64_t thread_id = 0; thread_id < num_threads; ++thread_id) {
        args[thread_id] = {0, thread_id, (chunked_table_t *) join_result->result, &next_chunk, l, p};
        int rv = pthread_create(&thread_ids[thread_id], nullptr, q19FilterChunkedThread, (void *) &args[thread_id]);
        if (rv) {
            logger(ERROR, "return code from pthread_create() is %d\n", rv);
        }
    }

    for (auto thread_id: thread_ids) {
        pthread_join(thread_id, nullptr);
    }

    uint64_t totalMatches = 0;
    for (const auto &arg: args) {
        totalMatches += arg.matches;
    }
    return totalMatches;
}

table_t
q19_lineitem_filter(uint64_t num_tuples, const row_t *orderkey, const type_key *partkey, const float *quantity,
                    const uint8_t *shipmode, const uint8_t *shipinstruct) {
    table_t filtered_table{};
    uint64_t selectionMatches = 0;

    filtered_table.tuples = (tuple_t *) aligned_alloc(64, sizeof(tuple_t) * num_tuples);
    malloc_check(filtered_table.tuples);

    // every register contains 64 predicate values
    auto predicate_vectors_8 = num_tuples / 64;

    __m512i shipmode_predicate_1 = _mm512_set1_epi8(L_SHIPMODE_AIR);
    __m512i shipmode_predicate_2 = _mm512_set1_epi8(L_SHIPMODE_AIR_REG);
    __m512i shipinstruct_predicate = _mm512_set1_epi8(L_SHIPINSTRUCT_DELIVER_IN_PERSON);
    __m512 l_quantity_predicate_low = _mm512_set1_ps(1);
    __m512 l_quantity_predicate_high = _mm512_set1_ps(30);

    auto shipmode_array_vec = reinterpret_cast<const __m512i *>(shipmode);
    auto shipinstruct_array_vec = reinterpret_cast<const __m512i *>(shipinstruct);
    auto quantity_array_vec = reinterpret_cast<const __m512 *>(quantity);

    for (uint64_t predicate_vector_i = 0; predicate_vector_i < predicate_vectors_8; ++predicate_vector_i) {
        __m512i shipmode_vec = _mm512_stream_load_si512((void *) (shipmode_array_vec + predicate_vector_i));
        __m512i shipinstruct_vec = _mm512_stream_load_si512((void *) (shipinstruct_array_vec + predicate_vector_i));

        __mmask64 mask = _mm512_cmpeq_epu8_mask(shipmode_vec, shipmode_predicate_1);
        mask = mask | _mm512_cmpeq_epu8_mask(shipmode_vec, shipmode_predicate_2);
        mask = mask & _mm512_cmpeq_epu8_mask(shipinstruct_vec, shipinstruct_predicate);

        if (mask == 0) {
            continue;
        }
        // Iteration for 4 bytes floats
        auto pos_32 = predicate_vector_i * 4;
        for (int k = 0; k < 4; ++k) {
            __mmask16 part_mask_shipmode_instruct = (mask >> k * 16) & 0xffff;

            if (part_mask_shipmode_instruct == 0) {
                continue;
            }

            __m512 quantity_vec = _mm512_load_ps((void *) (quantity_array_vec + pos_32 + k));
            __mmask16 quantity_mask = _mm512_cmpnlt_ps_mask(quantity_vec, l_quantity_predicate_low);
            quantity_mask = quantity_mask & _mm512_cmple_ps_mask(quantity_vec, l_quantity_predicate_high);

            __mmask16 mask_all = part_mask_shipmode_instruct & quantity_mask;

            if (mask_all == 0) {
                continue;
            }

            // I cannot be bothered to build table rows with SIMD right now.
            // Although it would work by interleaved merging of two 256 bit vectors. It is unclear if the unaligned
            // compress store used above even is better.
            auto pos_64 = (predicate_vector_i * 64 + k * 16);
            for (uint64_t bit = 0; bit < 16; ++bit, mask_all >>= 1) {
                if (mask_all & 1) {
                    filtered_table.tuples[selectionMatches] = {partkey[pos_64 + bit], orderkey[pos_64 + bit].payload};
                    selectionMatches++;
                }
            }
        }
    }

    // iterate over the remaining values from the last multiple of 64
    auto rest_start = num_tuples & ~(64 - 1);
    for (uint64_t i = rest_start; i < num_tuples; i++) {
        if ((quantity[i] >= 1 && quantity[i] <= (20 + 10)) &&
            (shipmode[i] == L_SHIPMODE_AIR || shipmode[i] == L_SHIPMODE_AIR_REG) &&
            (shipinstruct[i] == L_SHIPINSTRUCT_DELIVER_IN_PERSON)) {
            filtered_table.tuples[selectionMatches++] = {partkey[i], orderkey[i].payload};
        }
    }

    filtered_table.num_tuples = selectionMatches;
    return filtered_table;
}

table_t
q19_part_filter(uint64_t num_tuples, const row_t *partkey, const uint32_t *size, const uint8_t *brand,
                const uint8_t *container) {
    table_t filtered_table{};
    uint64_t selectionMatches = 0;

    filtered_table.tuples = (tuple_t *) aligned_alloc(64, sizeof(tuple_t) * num_tuples);
    malloc_check(filtered_table.tuples);

    // every register contains 64 predicate values
    auto predicate_vectors_8 = num_tuples / 64;

    __m512i brand_predicate_1 = _mm512_set1_epi8(P_BRAND_12);
    __m512i brand_predicate_2 = _mm512_set1_epi8(P_BRAND_23);
    __m512i brand_predicate_3 = _mm512_set1_epi8(P_BRAND_34);
    std::array<__m512i, 12> container_predicates{
            _mm512_set1_epi8(P_CONTAINER_SM_CASE),
            _mm512_set1_epi8(P_CONTAINER_SM_BOX),
            _mm512_set1_epi8(P_CONTAINER_SM_PACK),
            _mm512_set1_epi8(P_CONTAINER_SM_PKG),
            _mm512_set1_epi8(P_CONTAINER_MED_BAG),
            _mm512_set1_epi8(P_CONTAINER_MED_BOX),
            _mm512_set1_epi8(P_CONTAINER_MED_PKG),
            _mm512_set1_epi8(P_CONTAINER_MED_PACK),
            _mm512_set1_epi8(P_CONTAINER_LG_CASE),
            _mm512_set1_epi8(P_CONTAINER_LG_BOX),
            _mm512_set1_epi8(P_CONTAINER_LG_PACK),
            _mm512_set1_epi8(P_CONTAINER_LG_PKG),
    };
    __m512i size_predicate_low = _mm512_set1_epi32(1);
    __m512i size_predicate_high = _mm512_set1_epi32(15);

    auto brand_array_vec = reinterpret_cast<const __m512i *>(brand);
    auto container_array_vec = reinterpret_cast<const __m512i *>(container);
    auto size_array_vec = reinterpret_cast<const __m512i *>(size);
    auto input_array_vec = reinterpret_cast<const __m512i *>(partkey);
    auto output_buffer = filtered_table.tuples;

    for (uint64_t predicate_vector_i = 0; predicate_vector_i < predicate_vectors_8; ++predicate_vector_i) {
        __m512i brand_vec = _mm512_stream_load_si512((void *) (brand_array_vec + predicate_vector_i));
        __m512i container_vec = _mm512_stream_load_si512((void *) (container_array_vec + predicate_vector_i));

        __mmask64 mask_brand = _mm512_cmpeq_epu8_mask(brand_vec, brand_predicate_1) |
                               _mm512_cmpeq_epu8_mask(brand_vec, brand_predicate_2) |
                               _mm512_cmpeq_epu8_mask(brand_vec, brand_predicate_3);
        __mmask64 mask_container = 0;
        for (__m512i &predicate: container_predicates) {
            mask_container |= _mm512_cmpeq_epu8_mask(container_vec, predicate);
        }

        __mmask64 mask_64 = mask_brand & mask_container;

        if (mask_64 == 0) {
            continue;
        }
        // Iteration for 4 bytes integers
        auto pos_32 = predicate_vector_i * 4;
        for (int k = 0; k < 4; ++k) {
            __mmask16 part_mask = (mask_64 >> k * 16) & 0xffff;

            if (part_mask == 0) {
                continue;
            }

            __m512i size_vec = _mm512_stream_load_si512((void *) (size_array_vec + pos_32 + k));
            __mmask16 size_mask = _mm512_cmpge_epi32_mask(size_vec, size_predicate_low) &
                                  _mm512_cmple_epi32_mask(size_vec, size_predicate_high);
            __mmask16 mask_all = part_mask & size_mask;

            if (mask_all == 0) {
                continue;
            }

            /*
            // split mask into two 8 bit masks to use it for compress store
            auto pos_64 = pos_32 * 2;
            for (int j = 0; j < 2; ++j) {
                __mmask8 mask_8 = (mask_all >> j * 8) & 0xff;
                if (mask_8 == 0) {
                    continue;
                }
                _mm512_mask_compressstoreu_epi64(reinterpret_cast<void *>(output_buffer + selectionMatches),
                                                 mask_8,
                                                 _mm512_stream_load_si512((void *) (input_array_vec + pos_64 + j)));
                selectionMatches += __builtin_popcount(mask_8);
            }
             */
            // I cannot be bothered to build table rows with SIMD right now.
            // Although it would work by interleaved merging of two 256 bit vectors. It is unclear if the unaligned
            // compress store used above even is better.
            auto pos_64 = (predicate_vector_i * 64 + k * 16);
            for (uint64_t bit = 0; bit < 16; ++bit, mask_all >>= 1) {
                if (mask_all & 1) {
                    filtered_table.tuples[selectionMatches] = partkey[pos_64 + bit];
                    selectionMatches++;
                }
            }
        }
    }

    // iterate over the remaining values from the last multiple of 64
    auto rest_start = num_tuples & ~(64 - 1);
    for (uint64_t i = rest_start; i < num_tuples; i++) {
        if ((brand[i] == P_BRAND_12 || brand[i] == P_BRAND_23 || brand[i] == P_BRAND_34) &&
            (container[i] == P_CONTAINER_SM_CASE || container[i] == P_CONTAINER_SM_BOX ||
            container[i] == P_CONTAINER_SM_PACK || container[i] == P_CONTAINER_SM_PKG ||
            container[i] == P_CONTAINER_MED_BAG || container[i] == P_CONTAINER_MED_BOX ||
            container[i] == P_CONTAINER_MED_PKG || container[i] == P_CONTAINER_MED_PACK ||
            container[i] == P_CONTAINER_LG_CASE || container[i] == P_CONTAINER_LG_BOX ||
            container[i] == P_CONTAINER_LG_PACK || container[i] == P_CONTAINER_LG_PKG) &&
            (size[i] >= 1 && size[i] <= 15)) {
            filtered_table.tuples[selectionMatches++] = partkey[i];
        }
    }

    filtered_table.num_tuples = selectionMatches;
    return filtered_table;
}


#endif//Q19PREDICATES_HPP
