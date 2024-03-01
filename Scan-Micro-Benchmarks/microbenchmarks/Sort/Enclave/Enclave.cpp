#include "Enclave_t.h" // structs defined in .edl file etc

#include <vector>
#include <random>
#include <algorithm>
#include <sgx_trts.h>

#include "rdtscpWrapper.h"

constexpr int CACHE_LINE_SIZE = 64;

uint64_t *preload_data = nullptr;

extern "C" {

void ecall_noop() {
    //for benchmarking
}

void ecall_init_data(size_t num_elements) {
    try {
        preload_data = (uint64_t *) aligned_alloc(CACHE_LINE_SIZE, num_elements * sizeof(uint64_t));
    } catch (const std::bad_alloc &error) {
        ocall_print_error("Allocation inside enclave failed with std::bad_alloc!");
        throw error;
    }
}

void ecall_next_num_elements(size_t num_elements) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<uint64_t> distribution{};

    for (uint32_t i = 0; i < num_elements; ++i) { preload_data[i] = distribution(g); }
}

void ecall_sort(size_t num_elements, uint64_t *sort_cycle_counter) {
    rdtscpWrapper w{sort_cycle_counter};
    std::sort(preload_data, preload_data + num_elements);
}

void ecall_sort_user_check(uint64_t * o_data, size_t num_elements, uint64_t * sort_cycle_counter) {
    rdtscpWrapper w{sort_cycle_counter};
    std::sort(o_data, o_data + num_elements);
}

void ecall_free_preload_data() {
    free(preload_data);
    preload_data = nullptr;
}


}