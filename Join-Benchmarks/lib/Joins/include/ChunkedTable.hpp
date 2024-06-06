#ifndef SGXV2_JOIN_BENCHMARKS_CHUNKEDTABLE_HPP
#define SGXV2_JOIN_BENCHMARKS_CHUNKEDTABLE_HPP

#include "data-types.h"
#include <vector>

void
init_chunked_table(chunked_table_t *table);

void
init_chunked_table_prealloc(chunked_table_t *table, uint64_t num_chunks);

void
insert_output(chunked_table_t *table, type_key key, type_value Rpayload, type_value Spayload);

void
finish_chunked_table(chunked_table_t *table);

void
destroy_table(chunked_table_t *table);

chunked_table_t *
concatenate(const std::vector<chunked_table_t> &tables);

#endif//SGXV2_JOIN_BENCHMARKS_CHUNKEDTABLE_HPP
