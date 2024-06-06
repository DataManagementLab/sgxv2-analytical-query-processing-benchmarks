#ifndef SGX_ENCRYPTEDDB_TYPES_HPP
#define SGX_ENCRYPTEDDB_TYPES_HPP

#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "Enclave_u.h"

enum class Mode {
    bitvector = 0,
    noIndex = 1,
    dict = 2,
    scalar = 3,
};

constexpr std::array<const std::string_view, 4> mode_names{"bitvector", "noindex", "dict", "scalar"};

enum class Trinary { f = 0, t = 1, b = 2 };

[[nodiscard]] std::vector<bool>
convert_trinary(Trinary t);
[[nodiscard]] Trinary
from_bool(bool b);
[[nodiscard]] Trinary
from_string(const std::string &value);

template<class T>
bool
is_power_2(T n) {
    return (n & (n - 1)) == 0;
}

template<class T>
uint8_t
ilog2(T n) {
    return sizeof(T) * 8 - __builtin_clz(n) - 1;
}

template<class T>
struct ParameterSpan {
    const T min;
    const T max;
    const T step;
    const bool exp_steps;

private:
    T current;

public:
    ParameterSpan() = delete;
    ParameterSpan(T min, T max, T step, bool exp_steps)
        : min{min}, max{max}, step{step}, exp_steps{exp_steps}, current{min} {
        if (max < min) {
            throw std::invalid_argument("Max must be bigger than min!");
        }
        if (step <= 0) {
            throw std::invalid_argument("Step must be > 0!");
        }
        if (!exp_steps && (max - min) % step != 0) {
            throw std::invalid_argument("Min + n * step does not reach max!");
        }
        if (exp_steps) {
            if (!is_power_2(min) || !is_power_2(max)) {
                throw std::invalid_argument("Min and max must be powers of 2!");
            }
            if (this->step != 1 && ilog2(max / min) % step != 0) {
                throw std::invalid_argument("Min * 2^(n * step) does not reach max!");
            }
        }
    }

    [[nodiscard]] bool has_next() const {
        return current <= max;
    }

    T next() {
        T return_value = current;

        if (exp_steps) {
            this->current <<= step;
        } else {
            this->current += step;
        }

        return return_value;
    }

    [[nodiscard]] std::vector<T> to_vector() const {
        std::vector<T> result{};
        T local_current = min;
        while (local_current <= max) {
            result.push_back(local_current);
            if (exp_steps) {
                local_current <<= step;
            } else {
                local_current += step;
            }
        }
        return result;
    }
};

struct Configuration {
    bool enclave = false;
    bool preload = false;
    Mode mode = Mode::bitvector;
    MultiReadConfiguration read_config;
    bool NUMA = false;
    uint8_t num_threads = 1;
    uint64_t num_entries = 64;
    uint32_t num_reruns = 1;
    uint8_t selectivity = 10;
    bool include_join = false;
    bool pre_alloc = true;

    uint8_t predicate_low = 0;
    uint8_t predicate_high = 0;
    uint64_t num_entries_per_thread;

    Configuration()
        : read_config{true, 0, 1},
          predicate_high{static_cast<uint8_t>(std::round((static_cast<double>(selectivity) / 100.0) * 255.0))},
          num_entries_per_thread{num_entries / num_threads} {}

    Configuration(bool enclave, bool preload, Mode mode, MultiReadConfiguration read_config, bool numa,
                  uint8_t num_threads, uint64_t num_entries, uint64_t num_reruns, uint8_t selectivity,
                  bool include_join, bool pre_alloc)
        : enclave(enclave), preload(preload), mode(mode), read_config(read_config), NUMA(numa),
          num_threads(num_threads), num_entries(num_entries), num_reruns(num_reruns), selectivity(selectivity),
          include_join(include_join), pre_alloc(pre_alloc),
          predicate_high{static_cast<uint8_t>(std::round((static_cast<double>(selectivity) / 100.0) * 255.0))},
          num_entries_per_thread{num_entries / num_threads} {}
};

struct ConfigurationSpectrum {
    Trinary use_enclave = Trinary::f;
    Trinary preload = Trinary::f;
    std::vector<Mode> modes{};
    Trinary unique_data = Trinary::f;
    uint8_t num_warmup_runs = 0;
    uint32_t num_runs = 1;
    Trinary NUMA = Trinary::f;
    uint64_t num_reruns = 0;
    ParameterSpan<uint8_t> threads{1, 16, 1, true};
    ParameterSpan<uint64_t> num_entries{64, 64, 1, true};
    ParameterSpan<uint8_t> selectivity{1, 10, 1, false};
    bool join = false;
    bool pre_alloc = true;

    [[nodiscard]] std::vector<Configuration> enumerate() const {
        std::vector<bool> false_vector{};
        false_vector.push_back(false);
        std::vector<Configuration> result{};

        for (auto enclave: convert_trinary(use_enclave)) {
            for (auto preload: enclave ? convert_trinary(preload) : false_vector) {
                for (auto mode: modes) {
                    for (auto unique_data: convert_trinary(unique_data)) {
                        for (auto numa: convert_trinary(NUMA)) {
                            for (auto thrds: threads.to_vector()) {
                                for (auto ent: num_entries.to_vector()) {
                                    for (auto sel: selectivity.to_vector()) {
                                        Configuration config{
                                                enclave,
                                                preload,
                                                mode,
                                                {unique_data, static_cast<uint8_t>(!unique_data ? num_warmup_runs : 0), !unique_data ? num_runs : 1},
                                                numa,
                                                thrds,
                                                ent,
                                                num_reruns == 0 ? (num_entries.max / ent) : num_reruns,
                                                sel,
                                                join,
                                                pre_alloc};
                                        result.emplace_back(config);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }
};

#endif//SGX_ENCRYPTEDDB_TYPES_HPP