#ifndef SGX_ENCRYPTEDDB_MULTITHREADSCAN_UTILS_HPP
#define SGX_ENCRYPTEDDB_MULTITHREADSCAN_UTILS_HPP

#include <vector>
#include <thread>
#include <sgx_eid.h>
#include "types.hpp"

#include "../shared/ScanResults.hpp"

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(const char *enclave_file, sgx_enclave_id_t &enclave_id);

/* Destroy the enclave:
 *   Call sgx_destroy_enclave to destroy the enclave instance
 */
int destroy_enclave(sgx_enclave_id_t enclave_id);

bool setThreadAff(std::vector<std::thread> &threads, bool numa, uint32_t phys_cores_per_socket = 0);

bool join_counter_values(uint64_t &cycle_cntr, std::vector<uint64_t> &cpu_cntr_per_thread);

bool join_results(const Configuration &config, ScanResults &results, int thread_id);

void
reserve_index_scan_result_vectors(const Configuration &config, ScanResults &results);

void pre_alloc_index_scan_result_vectors(const Configuration &config, ScanResults &results, const uint8_t *data);

void
reserve_bitvector_result_vectors(const Configuration &config, ScanResults &results);

void
create_dict_result_vectors(const Configuration &config, ScanResults &results);

void pre_alloc_dict_scan_result_vectors(const Configuration &config, ScanResults &results, const uint8_t *data);

bool reserve_unified_result_vector(const Configuration &config, ScanResults &results);

#endif //SGX_ENCRYPTEDDB_MULTITHREADSCAN_UTILS_HPP
