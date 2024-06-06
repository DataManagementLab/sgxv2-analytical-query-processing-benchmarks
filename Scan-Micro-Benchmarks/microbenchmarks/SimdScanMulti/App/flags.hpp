#ifndef SGX_ENCRYPTEDDB_FLAGS_HPP
#define SGX_ENCRYPTEDDB_FLAGS_HPP

#include "types.hpp"
#include <gflags/gflags.h>
#include <vector>

DEFINE_string(enclave, "f", "Use enclave. Values: t, f, b");
DEFINE_string(preload, "f",
              "Whether to preload the enclave data or not. Values: t, f, b. "
              "(is ignored on unsafe)");
DEFINE_string(mode, "noIndex,bitvector", "Mode");
DEFINE_uint32(num_reruns, 0,
              "Set how often each vector size shall be scanned. Defaults to "
              "reading the maximum size. 'How often the exp is repeated'. If "
              "0, calculated as 16GByte/num-entries.");
DEFINE_string(unique_data, "t",
              "Whether to scan each data item only once or multiple times. "
              "Values: t, f, b.");
DEFINE_uint32(num_warmup_runs, 0, "Sets the number of warmup runs for in-cache scans.");
DEFINE_uint32(num_runs, 1,
              "Sets how often one data item shall be read if unique_data=false. 'How "
              "often the read is repeated within a run (number of cache reads)'");
DEFINE_string(numa, "f",
              "Whether to run the computation on the same node as the data "
              "resides. Values: t, f, b.");
DEFINE_uint32(min_threads, 1, "Minimal number of threads");
DEFINE_uint32(max_threads, 1, "Maximal number of threads");
DEFINE_uint64(min_entries, 1 << 12, "Minimum number of entries");
DEFINE_uint64(max_entries, 1 << 26, "Maximum number of entries");
DEFINE_uint64(min_entries_exp, 0, "Minimum number of entries in 2^n. Default 0 = use min_entries.");
DEFINE_uint64(max_entries_exp, 0, "Maximum number of entries in 2^n. Default 0 = use max_entries.");
DEFINE_uint32(max_selectivity, 10, "sets the maximum selectivity");
DEFINE_uint32(min_selectivity, 10, "sets the minimum selectivity");
DEFINE_uint32(step_selectivity, 1, "sets the selectivity step size");
DEFINE_bool(join, false, "Sets whether results of scan threads must be merged.");
DEFINE_bool(debug, false, "Toggle debug outputs.");
DEFINE_bool(pre_alloc, true,
            "Pre-allocate arrays for index and dictionary scan. Preventing "
            "them from dynamic allocation.");

static bool ValidateNumEntries(const char *flagname, uint64_t value) {
    if (value % 64 == 0) return true;
    printf("Invalid value for --%s: %lu, not multiple of 64 \n", flagname, value);
    return false;
}
static bool ValidateModes(const char *flagname, const std::string &value) {
    size_t start = 0;
    size_t comma_pos;
    do {
        comma_pos = value.find(',', start);
        if (comma_pos == std::string::npos) comma_pos = value.size();
        auto sub = value.substr(start, comma_pos - start);
        if (sub != "noIndex" && sub != "bitvector" && sub != "dict" && sub != "scalar") {
            printf("Illegal value for --%s: %s\n", flagname, value.c_str());
            return false;
        }
        start = comma_pos + 1;
    } while (comma_pos < value.size());

    return true;
}
static bool ValidateTrinary(const char *flagname, const std::string &value) {
    if (value == "t") return true;
    if (value == "f") return true;
    if (value == "b") return true;
    return false;
}

DEFINE_validator(min_entries, &ValidateNumEntries);
DEFINE_validator(mode, &ValidateModes);
DEFINE_validator(enclave, &ValidateTrinary);
DEFINE_validator(preload, &ValidateTrinary);
DEFINE_validator(unique_data, &ValidateTrinary);
DEFINE_validator(numa, &ValidateTrinary);

std::vector<Mode> parse_modes(const std::string &value) {
    std::vector<Mode> result{};

    size_t start = 0;
    size_t comma_pos;
    do {
        comma_pos = value.find(',', start);
        if (comma_pos == std::string::npos) comma_pos = value.size();
        auto sub = value.substr(start, comma_pos - start);

        if (sub == "noIndex") {
            result.push_back(Mode::noIndex);
        } else if (sub == "bitvector") {
            result.push_back(Mode::bitvector);
        } else if (sub == "dict") {
            result.push_back(Mode::dict);
        } else if (sub == "scalar") {
            result.push_back(Mode::scalar);
        }

        start = comma_pos + 1;
    } while (comma_pos < value.size());

    return result;
}

#endif// SGX_ENCRYPTEDDB_FLAGS_HPP
