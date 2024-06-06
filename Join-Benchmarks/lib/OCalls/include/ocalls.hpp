#ifndef SGXV2_JOIN_BENCHMARKS_OCALLS_HPP
#define SGXV2_JOIN_BENCHMARKS_OCALLS_HPP

#include <cstdint>

extern "C" {

void ocall_startTimer(uint64_t *t);

void ocall_stopTimer(uint64_t *t);

void ocall_start_performance_counters();

void ocall_stop_performance_counters(uint64_t scale_factor, const char *phase);

void ocall_get_system_micros(uint64_t *t);

void ocall_exit(int status);

void ocall_get_num_active_threads_in_numa(int *res, int numaregionid);

void ocall_get_thread_index_in_numa(int *res, int logicaltid);

void ocall_get_cpu_id(int *res, int thread_id);

void ocall_get_num_numa_regions(int *res);

void ocall_numa_thread_mark_active(int phytid);

void ocall_throw(const char *message);

void ocall_pin_thread(int tid);

void ocall_set_mask(uint64_t min_cid, uint64_t max_cid);

}

#endif //SGXV2_JOIN_BENCHMARKS_OCALLS_HPP
