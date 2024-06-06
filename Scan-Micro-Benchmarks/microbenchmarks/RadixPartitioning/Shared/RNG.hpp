#ifndef SGX_ENCRYPTEDDB_RNG_HPP
#define SGX_ENCRYPTEDDB_RNG_HPP

#include <cstdint>
#include <functional>
#include <random>

/**
 * b is exclusive
 */
static inline uint32_t uniform_int(uint64_t &seed, uint32_t a, uint32_t b) {
    seed = seed * 1103515245 + 12345; // LCG with standard params
    // return (uint32_t) (seed >> 32) % (b-a) + a; //modulo version. slower for
    // non-pow2 (b-a)s
    return (uint32_t) (((seed >> 32) * (b - a)) >> 32) +
           a; // multiply and shift version
}

// Fast random function using xorshift64* algorithm
class Xorshift64 {
private:
    uint64_t state;

public:
    explicit Xorshift64(uint64_t seed = 0xDEADBEEFDEADBEEF) : state(seed) {}

    uint64_t operator()() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }
};

class LinearCongruentialGenerator {
private:
    uint32_t state;

public:
    explicit LinearCongruentialGenerator(uint32_t seed = 0xDEADBEEF) : state(seed) {}

    uint32_t operator()() {
        constexpr uint32_t a = 1664525;
        constexpr uint32_t c = 1013904223;
        state = a * state + c;
        return state;
    }
};

class LinearCongruentialGenerator64 {
private:
    uint64_t state;

public:
    explicit LinearCongruentialGenerator64(uint64_t seed = 0xDEADBEEFULL) : state{seed} {}

    uint64_t operator()() {
        const uint64_t a = 2862933555777941757;
        const uint64_t c = 3037000493;
        state = a * state + c;
        return state;
    }
};

using RandomGeneratorFunction = std::function<void(uint64_t *, uint64_t, uint32_t *, uint32_t, std::mt19937&, bool)>;

enum class RandomMode {
    random = 0,
    random_linear = 1,
    linear = 2,
    ptr_linear = 3
};

RandomGeneratorFunction
choose_random_generator_function(RandomMode mode);

#endif //SGX_ENCRYPTEDDB_RNG_HPP
