#ifndef SGXV2_JOIN_BENCHMARKS_ENCLAVEPRINT_HPP
#define SGXV2_JOIN_BENCHMARKS_ENCLAVEPRINT_HPP

#if defined(__cplusplus)
extern "C" {
#endif

int printf(const char* fmt, ...);

//#ifndef logger
//#define logger(l, f_, ...) printf((f_), ##__VA_ARGS__)
//#endif

#if defined(__cplusplus)
}
#endif

#endif //SGXV2_JOIN_BENCHMARKS_ENCLAVEPRINT_HPP
