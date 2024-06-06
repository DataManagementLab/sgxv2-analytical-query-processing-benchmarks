#ifndef SGX_ENCRYPTEDDB_BARRIER_HPP
#define SGX_ENCRYPTEDDB_BARRIER_HPP

#include <atomic>

class Barrier {
private:
    const std::size_t threadCount;
    alignas(64) std::atomic<uint64_t> cntr;
    alignas(64) std::atomic<uint8_t> round;

public:
    explicit Barrier(std::size_t threadCount)
            : threadCount(threadCount), cntr(threadCount), round(0) {
    }

    template<typename F>
    bool wait(F finalizer) {
        auto prevRound = round.load();  // Must happen before fetch_sub
        auto prev = cntr.fetch_sub(1);
        if (prev == 1) {
            // last thread arrived
            cntr = threadCount;
            auto r = finalizer();
            round++;
            return r;
        } else {
            while (round == prevRound) {
                // wait until barrier is ready for re-use
                asm("pause");
                asm("pause");
                asm("pause");
            }
            return false;
        }
    }

    inline bool wait() {
        return wait([]() { return true; });
    }
};

#endif //SGX_ENCRYPTEDDB_BARRIER_HPP
