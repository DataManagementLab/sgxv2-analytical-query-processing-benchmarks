#include "ChunkedTable.hpp"
#include "Logger.hpp"
#include "util.hpp"
#include <algorithm>
#include <cstdlib>

#ifdef ENCLAVE
#include "ocalls_t.h"
#else
#include "ocalls.hpp"
#endif

void
add_chunk(chunked_table_t *table) {
    if (table->num_chunks == table->chunk_capacity) [[unlikely]] {
        // double capacity for chunk pointers
        // use realloc hoping that there is some free memory in place
        table->chunks = static_cast<table_chunk_t **>(
                realloc(table->chunks, table->chunk_capacity * sizeof(table_chunk_t *) * 2));
        malloc_check(table->chunks);
        table->chunk_capacity <<= 1;
    }
    auto new_chunk = reinterpret_cast<table_chunk_t *>(malloc(sizeof(table_chunk_t)));
    new_chunk->num_tuples = 0;
    // Memory of tuples not written to is undefined. No memset!
    table->chunks[table->num_chunks] = new_chunk;
    table->num_chunks++;
}
void
init_chunked_table(chunked_table_t *table) {
    if (table->chunk_capacity > 0) [[unlikely]] {
        logger(ERROR, "Trying to initialize a chunked table that is already initialized!");
        return;
    }
    table->num_tuples = 0;
    table->num_chunks = 0;
    table->chunks = static_cast<table_chunk_t **>(aligned_alloc(64, 64)); // allocate exactly one cache line
    table->chunk_capacity = 64 / sizeof(table_chunk_t *); // should be 8
    add_chunk(table); // add the first chunk
}
void
insert_output(chunked_table_t *table, type_key key, type_value Rpayload, type_value Spayload) {
    auto chunk = table->chunks[table->num_chunks - 1];
    if (chunk->num_tuples >= TUPLES_PER_CHUNK) [[unlikely]] {
        // last chunk full
        add_chunk(table);
        chunk = table->chunks[table->num_chunks - 1]; // after adding a new chunk, move chunk pointer to it.
    }

    chunk->tuples[chunk->num_tuples].key = key;
    chunk->tuples[chunk->num_tuples].Rpayload = Rpayload;
    chunk->tuples[chunk->num_tuples].Spayload = Spayload;
    chunk->num_tuples++;
}
void
finish_chunked_table(chunked_table_t *table) {
    for (uint64_t i = 0; i < table->num_chunks; ++i) {
        table->num_tuples += table->chunks[i]->num_tuples;
    }
} /**
 * Frees all chunks and the chunk pointer array. Does not free the table itself.
 * @param table
 */
void
destroy_table(chunked_table_t *table) {
    for (uint64_t i = 0; i < table->num_chunks; ++i) {
        free(table->chunks[i]);
    }
    free(table->chunks);
    table->chunk_capacity = 0;
    table->num_chunks = 0;
}
chunked_table_t *
concatenate(const std::vector<chunked_table_t> &tables) {
    auto result_table = static_cast<chunked_table_t *>(malloc(sizeof(chunked_table_t)));

    // Calculate required number of chunks and number of tuples
    result_table->num_chunks = 0;
    result_table->num_tuples = 0;
    for (const chunked_table_t &table : tables) {
        result_table->num_chunks += table.num_chunks;
        result_table->num_tuples += table.num_tuples;
    }

    // Allocate array for chunk pointers
    result_table->chunks = (table_chunk_t **) malloc(sizeof(table_chunk_t *) * result_table->num_chunks);
    result_table->chunk_capacity = result_table->num_chunks;

    // Copy chunk pointers
    uint64_t start = 0;
    for (const chunked_table_t &table: tables) {
        std::copy_n(table.chunks, table.num_chunks, result_table->chunks + start);
        start += table.num_chunks;
    }
    return result_table;
}