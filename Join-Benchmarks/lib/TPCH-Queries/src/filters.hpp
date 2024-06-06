#ifndef SGXV2_JOIN_BENCHMARKS_FILTERS_HPP
#define SGXV2_JOIN_BENCHMARKS_FILTERS_HPP

#include "Barrier.hpp"
#include "Logger.hpp"
#include "Q19Predicates.hpp"
#include "TpcHTypes.hpp"

#include <algorithm>
#include <pthread.h>
#include <tuple>

template<typename... Args>
using FilterFunctionType = table_t (*)(uint64_t, Args...);

template<typename... Args>
struct parallel_filter_parameters {
    std::vector<table_t> *results;
    Barrier *b;
    table_t *result_table;
    uint64_t num_threads;
    std::vector<uint64_t> *thread_start_positions;
    uint64_t thread_id;
    uint64_t tuple_count;
    FilterFunctionType<Args...> filter_function;
    std::tuple<Args...> var_args;
};

template<typename... Args>
table_t
parallel_filter(uint64_t num_threads, FilterFunctionType<Args...> filter_function, uint64_t num_tuples, Args... args) {
    auto tuples_per_thread = num_tuples / num_threads;
    tuples_per_thread = tuples_per_thread & ~63;// make sure that tuples_per_thread is divisible by 64
    auto tuples_last_thread = num_tuples - (tuples_per_thread * (num_threads - 1));

    std::vector<table_t> results(num_threads);
    std::vector<pthread_t> worker_threads(num_threads - 1);
    std::vector<uint64_t> thread_start_positions(num_threads);
    table_t result_table{};
    Barrier b{num_threads};

    auto thread_function = [](void *arg_void_ptr) -> void * {
        auto parameters = reinterpret_cast<parallel_filter_parameters<Args...> *>(arg_void_ptr);
        // Each thread filters its results
        // Let's hope copy elision works as intended here...

        auto filter_function_varargs = [tuple_count = parameters->tuple_count,
                                        f_function = parameters->filter_function](Args... varargs) {
            return f_function(tuple_count, varargs...);
        };

        parameters->results->at(parameters->thread_id) = std::apply(filter_function_varargs, parameters->var_args);

        // last thread calculates number of tuples, start positions for writes and allocates memory
        parameters->b->wait([parameters] {
            uint64_t num_t = parameters->num_threads;
            for (uint64_t tid = 0; tid < num_t; ++tid) {
                parameters->thread_start_positions->at(tid) = parameters->result_table->num_tuples;
                parameters->result_table->num_tuples += parameters->results->at(tid).num_tuples;
            }

            parameters->result_table->tuples =
                    reinterpret_cast<row_t *>(malloc(parameters->result_table->num_tuples * sizeof(row_t)));

            return true;
        });

        // All threads copy their tuples to the right position in the output table
        std::copy_n(parameters->results->at(parameters->thread_id).tuples,
                    parameters->results->at(parameters->thread_id).num_tuples,
                    parameters->result_table->tuples + parameters->thread_start_positions->at(parameters->thread_id));

        free(parameters->results->at(parameters->thread_id).tuples);
        return nullptr;
    };

    std::vector<parallel_filter_parameters<Args...>> thread_params{};

    // Create parameters for the worker threads
    for (uint64_t thread_id = 1; thread_id < num_threads; ++thread_id) {
        uint64_t start_index = tuples_per_thread * thread_id;
        uint64_t tuples_to_scan = thread_id < (num_threads - 1) ? tuples_per_thread : tuples_last_thread;
        thread_params.push_back({&results, &b, &result_table, num_threads, &thread_start_positions, thread_id,
                                 tuples_to_scan, filter_function, std::tuple<Args...>{(args + start_index)...}});
    }

    // Start worker threads (cannot be combined with previous loop since the previous loop is increasing the vector size)
    for (uint64_t worker_thread_id = 0; worker_thread_id < (num_threads - 1); ++worker_thread_id) {
        pthread_create(&worker_threads[worker_thread_id], nullptr, thread_function,
                       (void *) &thread_params[worker_thread_id]);
    }

    // Do the first work package on the main thread
    parallel_filter_parameters<Args...> parameters{&results,
                                                   &b,
                                                   &result_table,
                                                   num_threads,
                                                   &thread_start_positions,
                                                   0,
                                                   num_threads > 1 ? tuples_per_thread : tuples_last_thread,
                                                   filter_function,
                                                   std::tuple<Args...>{args...}};
    thread_function((void *) &parameters);

    // Synchronize and wait for all worker threads to finish
    for (auto worker_thread: worker_threads) {
        pthread_join(worker_thread, nullptr);
    }

    return result_table;
}

template<class TableType>
using FilterPredicateType = bool (*)(const TableType &, uint64_t);
template<class TableType>
using CopyFunctionType = void (*)(const TableType &, table_t &, uint64_t, uint64_t);

template<class TableType, FilterPredicateType<TableType> FilterPredicate, CopyFunctionType<TableType> CopyFunction>
table_t
filter_table(const TableType &table) {
    table_t filtered_table{};

    uint64_t selectionMatches = 0;
    filtered_table.tuples = (tuple_t *) malloc(sizeof(tuple_t) * table.numTuples);
    malloc_check(filtered_table.tuples);
    for (uint64_t i = 0; i < table.numTuples; i++) {
        if (FilterPredicate(table, i)) {
            CopyFunction(table, filtered_table, i, selectionMatches++);
        }
    }
    filtered_table.num_tuples = selectionMatches;
    if (selectionMatches < table.numTuples) {
        filtered_table.tuples = (tuple_t *) realloc(filtered_table.tuples, (sizeof(tuple_t) * selectionMatches));
        malloc_check(filtered_table.tuples);
    }

    return filtered_table;
}


#endif//SGXV2_JOIN_BENCHMARKS_FILTERS_HPP
