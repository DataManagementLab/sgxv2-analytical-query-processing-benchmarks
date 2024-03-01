#ifndef OCALLS_T_H__
#define OCALLS_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


sgx_status_t SGX_CDECL ocall_get_system_micros(uint64_t* retval);
sgx_status_t SGX_CDECL ocall_startTimer(uint64_t* t);
sgx_status_t SGX_CDECL ocall_stopTimer(uint64_t* t);
sgx_status_t SGX_CDECL ocall_start_performance_counters(void);
sgx_status_t SGX_CDECL ocall_stop_performance_counters(uint64_t scale_factor, const char* phase);
sgx_status_t SGX_CDECL ocall_exit(int exit_status);
sgx_status_t SGX_CDECL ocall_throw(const char* message);
sgx_status_t SGX_CDECL ocall_get_num_active_threads_in_numa(int* retval, int numaregionid);
sgx_status_t SGX_CDECL ocall_get_thread_index_in_numa(int* retval, int logicaltid);
sgx_status_t SGX_CDECL ocall_get_cpu_id(int* retval, int thread_id);
sgx_status_t SGX_CDECL ocall_get_num_numa_regions(int* retval);
sgx_status_t SGX_CDECL ocall_numa_thread_mark_active(int phytid);
sgx_status_t SGX_CDECL ocall_pin_thread(int tid);
sgx_status_t SGX_CDECL ocall_set_mask(uint64_t min_cid, uint64_t max_cid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
