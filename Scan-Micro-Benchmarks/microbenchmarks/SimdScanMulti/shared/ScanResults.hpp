#ifndef SGX_ENCRYPTEDDB_SCANRESULTS_HPP
#define SGX_ENCRYPTEDDB_SCANRESULTS_HPP

#include <vector>
#include <Allocator.hpp>

template<typename T>
using CacheAlignedVector = std::vector<T, AlignedAllocator<T>>;

struct ScanResults {
    std::vector<uint64_t> index_results; // Unified vector containing the scan results in order
    std::vector<CacheAlignedVector<uint64_t>> index_results_per_thread; // Intermediate scan results of each thread
    std::vector<int64_t> dict_results; // Unified vector containing the scan results in order
    std::vector<CacheAlignedVector<int64_t>> dict_results_per_thread; // Intermediate scan results of each thread
    std::vector<uint64_t> bitvector_results; // Unified vector containing the scan results in order
};


#endif //SGX_ENCRYPTEDDB_SCANRESULTS_HPP
