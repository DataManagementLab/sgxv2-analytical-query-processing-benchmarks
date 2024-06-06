#ifndef SGX_ENCRYPTEDDB_RDTSCPWRAPPER_H
#define SGX_ENCRYPTEDDB_RDTSCPWRAPPER_H


class rdtscpWrapper {
private:
    inline uint64_t rdtscp() {
        uint32_t aux;
        uint64_t rax;
        uint64_t rdx;
        asm volatile("rdtscp\n" : "=a"(rax), "=d"(rdx), "=c"(aux) : :);
        return (rdx << 32) + rax;
    }
    uint64_t *cntr;
    uint64_t start;
    uint64_t end;

public:
    explicit rdtscpWrapper(uint64_t *cntr) : cntr{cntr}, start{rdtscp()} {}

    ~rdtscpWrapper() {
        end = rdtscp();
        *cntr += end - start;
    }
};


#endif//SGX_ENCRYPTEDDB_RDTSCPWRAPPER_H
