#include "PerfEvent.hpp"
#include "rdtscpWrapper.h"
#include "sgxerrors.h"
#include <algorithm>
#include <gflags/gflags.h>
#include <iostream>
#include <random>

#include "Enclave_u.h"

#include "Logger.hpp"

constexpr char ENCLAVE_FILENAME[] = "sortenc.signed.so";

DEFINE_int32(min_size_exp, 4, "Minimum number of values to sort.");
DEFINE_int32(max_size_exp, 27, "Maximum number of values to sort.");
DEFINE_bool(debug, false, "Debug output on/off");
DEFINE_bool(user_check, true, "User check mode on/off");
DEFINE_uint32(repeat, 1, "How often to rerun each setting.");

constexpr int CACHE_LINE_SIZE = 64;
constexpr int L1_CACHE_SIZE = (48 * 1024);
constexpr int L2_CACHE_SIZE = (1280 * 1024);

Logger l{};


int initialize_enclave(const char *enclave_file, sgx_enclave_id_t &enclave_id) {
    sgx_status_t ret;

    l.log("Initializing enclave.");

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(enclave_file, 1, nullptr, nullptr, &enclave_id, nullptr);
    if (ret != SGX_SUCCESS) {
        l.err("Enclave initialization failed!");
        print_error_message(ret);
        return -1;
    }
    ret = ecall_noop(enclave_id);
    if (ret != SGX_SUCCESS) {
        l.err("First call into enclave failed!");
        print_error_message(ret);
        return -1;
    }
    l.log("Done.");
    return 0;
}

int destroy_enclave(sgx_enclave_id_t enclave_id) {
    sgx_status_t ret;

    l.log("Destroying enclave.");

    /* Call sgx_destroy_enclave to destroy  the enclave instance */
    ret = sgx_destroy_enclave(enclave_id);
    if (ret != SGX_SUCCESS) {
        l.err("Destroying the enclave failed!");
        print_error_message(ret);
        return -1;
    }
    l.log("Done.");
    return 0;
}

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage("Radix partitioning benchmark");
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    l.debug = FLAGS_debug;

    sgx_enclave_id_t eid = 0;
    auto ret = initialize_enclave(ENCLAVE_FILENAME, eid);
    if (ret != 0) { return -1; }

    auto data = (uint64_t *) std::aligned_alloc(CACHE_LINE_SIZE, (1 << FLAGS_max_size_exp) * sizeof(uint64_t));
    ret = ecall_init_data(eid, (1 << FLAGS_max_size_exp));
    if (ret != SGX_SUCCESS) { l.err("init data failed"); }
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<uint64_t> distribution{};

    bool print_header = true;
    for (uint32_t num_elements_exp = FLAGS_min_size_exp; num_elements_exp <= FLAGS_max_size_exp; ++num_elements_exp) {
        size_t num_elements = 1 << num_elements_exp;
        for (uint32_t repeat = 0; repeat < FLAGS_repeat; ++repeat) {

            for (uint32_t i = 0; i < num_elements; ++i) { data[i] = distribution(g); }

            BenchmarkParameters params{};
            params.setParam("num_elements", num_elements);
            params.setParam("data_size", num_elements * sizeof(uint64_t));
            params.setParam("workload", "sort");
            params.setParam("sgx", "no");
            params.setParam("preload", "no");

            {
                uint64_t sort_cycle_counter = 0;
                PerfEventBlock e{num_elements, params, print_header, &sort_cycle_counter};
                rdtscpWrapper w{&sort_cycle_counter};
                std::sort(data, data + num_elements);
            }
            print_header = false;

            if (FLAGS_user_check) {
                for (uint32_t i = 0; i < num_elements; ++i) { data[i] = distribution(g); }
                params.setParam("sgx", "yes");
                params.setParam("preload", "no");

                uint64_t sort_cycle_counter_user_check = 0;
                {
                    PerfEventBlock e{num_elements, params, print_header, &sort_cycle_counter_user_check};
                    ret = ecall_sort_user_check(eid, data, num_elements, &sort_cycle_counter_user_check);
                    if (ret != SGX_SUCCESS) { l.err("ecall_sort_user_check failed"); }
                }
            }

            params.setParam("sgx", "yes");
            params.setParam("preload", "yes");
            ret = ecall_next_num_elements(eid, num_elements);
            if (ret != SGX_SUCCESS) { l.err("ecall_next_num_elements failed"); }
            uint64_t sort_cycle_counter_enclave = 0;
            {
                PerfEventBlock e{num_elements, params, print_header, &sort_cycle_counter_enclave};
                ret = ecall_sort(eid, num_elements, &sort_cycle_counter_enclave);
                if (ret != SGX_SUCCESS) { l.err("ecall_sort failed"); }
            }
        }
    }

    // One of these somehow causes an error on munmap().
    //std::free(partitioned);
    //std::free(data);

    ret = ecall_free_preload_data(eid);
    if (ret != SGX_SUCCESS) { l.err("ecall_free_preload_data failed"); }
    destroy_enclave(eid);
    return 0;
}
