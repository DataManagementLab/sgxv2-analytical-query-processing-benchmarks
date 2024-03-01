#ifndef SGXV2_JOIN_BENCHMARKS_RESULT_TRANSFORMERS_HPP
#define SGXV2_JOIN_BENCHMARKS_RESULT_TRANSFORMERS_HPP

#include "data-types.h"
#include "Logger.hpp"
#include <cstdlib>

using ToTableThread = void *(*) (void *param);

void *
toTableThread_RpToKeySpST(void *param);

void *
toTableThread_RpToKeySp(void *param);

void *
toTableThread_SpToTupleST(void *param);

void *
toTableThread_SpToTuple(void *param);

void *
toTableThread_SpSpST(void *param);

void *
toTableThread_SpSp(void *param);

void *
toTableThread_RkSp(void *param);

void
joinResultToTableST(table_t *table, const result_t *jr, ToTableThread t_thread, const type_key *keyToExtract = nullptr);

void
joinResultToTable(table_t *table, const result_t *jr, ToTableThread t_thread, const type_key *keyToExtract = nullptr);


void
joinResultToTableST(table_t *table, const result_t *jr, ToTableThread t_thread, const tuple_t *keyToExtract,
                    uint64_t keySize);

void
joinResultToTable(table_t *table, const result_t *jr, ToTableThread t_thread, const tuple_t *keyToExtract,
                  uint64_t keySize);

using TripleToTupleFunctionType = row_t (*)(const output_triple_t &);
template<typename LookupTableType>
using TripleToTupleLookupFunctionType = row_t (*)(const output_triple_t &, const LookupTableType *);

[[gnu::always_inline, nodiscard]] inline row_t
copy_Sp_Sp(const output_triple_t &triple) {
    return {triple.Spayload, triple.Spayload};
}

template<typename LookupTableType>
[[gnu::always_inline, nodiscard]] inline row_t
copy_RpToKeySp(const output_triple_t &triple, const LookupTableType *lookup_table) {
    return {lookup_table[triple.Rpayload], triple.Spayload};
}

[[gnu::always_inline, nodiscard]] inline row_t
copy_SpToTupleST(const output_triple_t &triple, const tuple_t *lookup_table) {
    return {lookup_table[triple.Spayload].key, 0};
}

/**
 * Copies all tuples from all chunks to the returned solid table. Uses copy_function to transform triples to tuples.
 * TODO make copy_function compile time polymorph to allow inlining. All in all, this function is similar to the
 *  other translation functions in TpcHECalls
 * TODO enable parallel copy of chunks in threads
 * @param input_table
 * @param copy_function
 * @return
 */
template<TripleToTupleFunctionType CopyFunction>
void
chunked_to_solid_table(table_t *result_table, const chunked_table_t *input_table) {
    // Initialize result table
    result_table->num_tuples = input_table->num_tuples;
    result_table->tuples = static_cast<row_t *>(malloc(sizeof(row_t) * input_table->num_tuples));
    result_table->ratio_holes = 0;
    result_table->sorted = 0;

    // Copy all tuples from all chunks over
    uint64_t output_row_index = 0;
    for (uint64_t chunk_index = 0; chunk_index < input_table->num_chunks; ++chunk_index) {
        auto chunk = input_table->chunks[chunk_index];
        for (uint64_t tuple_index = 0; tuple_index < chunk->num_tuples; ++tuple_index) {
            result_table->tuples[output_row_index++] = CopyFunction(chunk->tuples[tuple_index]);
        }
    }
    if (output_row_index != input_table->num_tuples) {
        logger(ERROR, "Chunked to solid table failed!");
    }
}

/**
 * Copies all tuples from all chunks to the returned solid table. Uses copy_function to transform triples to tuples.
 * TODO make copy_function compile time polymorph to allow inlining. All in all, this function is similar to the
 *  other translation functions in TpcHECalls
 * TODO enable parallel copy of chunks in threads
 * @param input_table
 * @param copy_function
 * @return
 */
template<typename LookupTableType, TripleToTupleLookupFunctionType<LookupTableType> CopyFunction>
void
chunked_to_solid_table(table_t *result_table, const chunked_table_t *input_table, const LookupTableType *lookup_table) {
    // Initialize result table
    result_table->num_tuples = input_table->num_tuples;
    result_table->tuples = static_cast<row_t *>(malloc(sizeof(row_t) * input_table->num_tuples));
    result_table->ratio_holes = 0;
    result_table->sorted = 0;

    // Copy all tuples from all chunks over
    uint64_t output_row_index = 0;
    for (uint64_t chunk_index = 0; chunk_index < input_table->num_chunks; ++chunk_index) {
        auto chunk = input_table->chunks[chunk_index];
        for (uint64_t tuple_index = 0; tuple_index < chunk->num_tuples; ++tuple_index) {
            result_table->tuples[output_row_index++] = CopyFunction(chunk->tuples[tuple_index], lookup_table);
        }
    }

    if (output_row_index != input_table->num_tuples) {
        logger(ERROR, "Chunked to solid table failed!");
    }
}

#endif//SGXV2_JOIN_BENCHMARKS_RESULT_TRANSFORMERS_HPP
