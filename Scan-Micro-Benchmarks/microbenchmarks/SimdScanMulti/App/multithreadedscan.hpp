#ifndef SGX_ENCRYPTEDDB_MULTITHREADEDSCAN_HPP
#define SGX_ENCRYPTEDDB_MULTITHREADEDSCAN_HPP

#include <sgx_eid.h>
#include <vector>
#include <string>

#include "../shared/Barrier.hpp"
#include "types.hpp"

#include "../shared/ScanResults.hpp"

int
run_bitvector_scan(const uint8_t *data,
                   uint64_t start_idx,
                   ScanResults &results,
                   uint64_t output_idx,
                   uint64_t &cpu_cycles_cntr,
                   const Configuration &config,
                   sgx_enclave_id_t eid,
                   Barrier &barrier,
                   int thread_id);

int
run_index_scan(const uint8_t *data,
               uint64_t start_idx,
               ScanResults &results,
               uint64_t &cpu_cycles_cntr,
               const Configuration &config,
               sgx_enclave_id_t eid,
               Barrier &barrier,
               int thread_id);

int
run_dict_scan(const uint8_t *data,
              uint64_t start_idx,
              const int64_t *dict,
              ScanResults &results,
              uint64_t &cpu_cycles_cntr,
              const Configuration &config,
              sgx_enclave_id_t eid,
              Barrier &barrier,
              int thread_id);

void
scan_wrapper(int t, const Configuration &config, ScanResults &results, const uint8_t *data, const int64_t *dict,
             Barrier &barrier, uint64_t &cycle_cntr, std::vector<uint64_t> &cpu_cntr_per_thread,
             sgx_enclave_id_t global_eid);

#endif //SGX_ENCRYPTEDDB_MULTITHREADEDSCAN_HPP
