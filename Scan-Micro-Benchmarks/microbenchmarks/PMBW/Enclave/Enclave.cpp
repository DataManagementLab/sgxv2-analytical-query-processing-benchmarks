#include "Enclave_t.h" // structs defined in .edl file etc
#include <cstring> // for memset


/******************************************************************************
 * pmbw.cc
 *
 * Parallel Memory Bandwidth Measurement / Benchmark Tool.
 *
 * The main program creates threads using the pthread library and calls the
 * assembler functions appropriate for the platform. It also uses CPUID to
 * detect which routines are applicable. The benchmark results are always
 * outputted to "stats.txt" which can then be processed using other tools.
 *
 ******************************************************************************
 * Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <iostream>
#include <vector>
#include <cstdlib>
#include <string>

#include <pthread.h>

#include "Barrier.hpp"
#include "structs.h"
#include "rdtscpWrapper.h"

// -----------------------------------------------------------------------------
// --- Global Settings and Variables

// minimum duration of test, if smaller re-run
double g_min_time = 1.0;

// target average duration of test
double g_avg_time = 1.5;

// filter of functions to run, set by command line
std::vector<const char *> gopt_funcfilter;

// set default size limit: 0 -- 4 GiB
uint64_t gopt_sizelimit_min;
uint64_t gopt_sizelimit_max;

// set memory limit
uint64_t gopt_memlimit;

// lower and upper limit to number of threads
int gopt_nthreads_min, gopt_nthreads_max;

// exponentially increasing number of threads
bool gopt_nthreads_exponential;

// option to test permutation cycle before measurement
bool gopt_testcycle;



// error writers
#define ERR(x)  do { ocall_print_string(x);} while(0)
#define ERRX(x)  do { (std::cerr << x).flush(); } while(0)

// allocated memory area and size
char *g_memarea = NULL;
size_t g_memsize = 0;

// global test function currently run
const struct TestFunction *g_func = NULL;

// number of physical cpus detected
int g_physical_cpus;

// hostname
char g_hostname[256];


#define REGISTER(func, bytes, offset, unroll)                   \
    static const struct TestFunction* _##func##_register =       \
        new TestFunction(#func,func,NULL,bytes,offset,unroll,false);

#define REGISTER_CPUFEAT(func, cpufeat, bytes, offset, unroll)  \
    static const struct TestFunction* _##func##_register =       \
        new TestFunction(#func,func,cpufeat,bytes,offset,unroll,false);

#define REGISTER_PERM(func, bytes)                              \
    static const struct TestFunction* _##func##_register =       \
        new TestFunction(#func,func,NULL,bytes,bytes,1,true);

// -----------------------------------------------------------------------------
// --- Test Functions with Inline Assembler Loops

#if __x86_64__

#include "../incl/funcs_x86_64.h"

#elif defined(__i386__)
#include "funcs_x86_32.h"
#elif __aarch64__
#include "funcs_arm64.h"
#elif __arm__
#include "funcs_arm.h"
#else
#include "funcs_c.h"
#endif

// -----------------------------------------------------------------------------
// --- Test CPU Features via CPUID

#if defined(__i386__) || defined (__x86_64__)

//  gcc inline assembly for CPUID instruction
static inline void cpuid(int op, int out[4]) {
    asm volatile("cpuid"
            : "=a" (out[0]), "=b" (out[1]), "=c" (out[2]), "=d" (out[3])
            : "a" (op)
            );
}

// cpuid op 1 result
int g_cpuid_op1[4];

// check for MMX instructions
static bool cpuid_mmx() {
    return (g_cpuid_op1[3] & ((int) 1 << 23));
}

// check for SSE instructions
static bool cpuid_sse() {
    return (g_cpuid_op1[3] & ((int) 1 << 25));
}

// check for AVX instructions
static bool cpuid_avx() {
    return (g_cpuid_op1[2] & ((int) 1 << 28));
    //return false;
}

// check for AVX512 instructions
static bool cpuid_avx512() {
    // TODO make this a real check
    return true;
}

// run CPUID and print output
static void cpuid_detect() {
    ocall_print_string("CPUID:");
    ocall_cpuid_detect(g_cpuid_op1);

    if (cpuid_mmx()) ocall_print_string(" mmx");
    if (cpuid_sse()) ocall_print_string(" sse");
    if (cpuid_avx()) ocall_print_string(" avx");
    if (cpuid_avx512()) ocall_print_string(" avx512");
    ocall_print_string("");
}

// TestFunction feature detection
bool TestFunction::is_supported() const {
    if (!cpufeat) return true;
    if (strcmp(cpufeat, "mmx") == 0) return cpuid_mmx();
    if (strcmp(cpufeat, "sse") == 0) return cpuid_sse();
    if (strcmp(cpufeat, "avx") == 0) return cpuid_avx();
    if (strcmp(cpufeat, "avx512") == 0) return cpuid_avx512();
    return false;
}

#else
static void cpuid_detect()
{
    // replace functions with dummys
}
bool TestFunction::is_supported() const
{
    return true;
}
#endif

// -----------------------------------------------------------------------------
// --- Some Simple Subroutines

// parse a number as size_t with error detection
static inline bool
parse_uint64t(const char *value, uint64_t &out) {
    char *endp;
    out = strtoull(value, &endp, 10);
    if (!endp) return false;
    // read additional suffix
    if (*endp == 'k' || *endp == 'K') {
        out *= 1024;
        ++endp;
    } else if (*endp == 'm' || *endp == 'M') {
        out *= 1024 * 1024;
        ++endp;
    } else if (*endp == 'g' || *endp == 'G') {
        out *= 1024 * 1024 * 1024llu;
        ++endp;
    } else if (*endp == 't' || *endp == 'T') {
        out *= 1024 * 1024 * 1024 * 1024llu;
        ++endp;
    }
    return (endp && *endp == 0);
}

// parse a number as int with error detection
static inline bool
parse_int(const char *value, int &out) {
    char *endp;
    out = strtoul(value, &endp, 10);
    if (!endp) return false;
    // read additional suffix
    if (*endp == 'k' || *endp == 'K') {
        out *= 1024;
        ++endp;
    } else if (*endp == 'm' || *endp == 'M') {
        out *= 1024 * 1024;
        ++endp;
    } else if (*endp == 'g' || *endp == 'G') {
        out *= 1024 * 1024 * 1024llu;
        ++endp;
    } else if (*endp == 't' || *endp == 'T') {
        out *= 1024 * 1024 * 1024 * 1024llu;
        ++endp;
    }
    return (endp && *endp == 0);
}

// Simple linear congruential random generator
struct LCGRandom {
    uint64_t xn;

    inline LCGRandom(uint64_t seed) : xn(seed) {}

    inline uint64_t operator()() {
        xn = 0x27BB2EE687B0B0FDLLU * xn + 0xB504F32DLU;
        return xn;
    }
};


// return true if the funcname is selected via command line arguments
static inline bool match_funcfilter(const char *funcname) {
    if (gopt_funcfilter.size() == 0) return true;

    // iterate over gopt_funcfilter list
    for (size_t i = 0; i < gopt_funcfilter.size(); ++i) {
        if (strstr(funcname, gopt_funcfilter[i]) != NULL)
            return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// --- List of Array Sizes to Test

const uint64_t areasize_list[] = {
        1 * 1024,                   // 1 KiB
        2 * 1024,
        3 * 1024,
        4 * 1024,
        6 * 1024,
        8 * 1024,
        12 * 1024,
        16 * 1024,
        20 * 1024,
        24 * 1024,
        28 * 1024,
        32 * 1024,
        40 * 1024,
        48 * 1024,
        64 * 1024,
        96 * 1024,
        128 * 1024,
        192 * 1024,
        256 * 1024,
        384 * 1024,
        512 * 1024,
        768 * 1024,
        1024 * 1024,                // 1 MiB
        (1024 + 256) * 1024,    // 1.25 MiB
        (1024 + 512) * 1024,    // 1.5 MiB
        (1024 + 768) * 1024,    // 1.75 MiB
        2048 * 1024,                // 2 MiB = common L2 cache size
        (2048 + 256) * 1024,    // 2.25
        (2048 + 512) * 1024,    // 2.5
        (2048 + 768) * 1024,    // 2.75
        3 * 1024 * 1024,            // 3 MiB = common L2 cache size
        4 * 1024 * 1024,            // 4 MiB
        5 * 1024 * 1024,            // 5 MiB
        6 * 1024 * 1024,            // 6 MiB = common L2 cache size
        7 * 1024 * 1024,            // 7 MiB
        8 * 1024 * 1024,            // 8 MiB = common L2 cache size
        9 * 1024 * 1024,
        10 * 1024 * 1024,
        12 * 1024 * 1024,
        14 * 1024 * 1024,
        16 * 1024 * 1024,
        20 * 1024 * 1024,
        24 * 1024 * 1024,
        28 * 1024 * 1024,
        32 * 1024 * 1024,
        40 * 1024 * 1024,
        48 * 1024 * 1024,
        56 * 1024 * 1024,
        64 * 1024 * 1024,
        128 * 1024 * 1024,
        256 * 1024 * 1024,
        512 * 1024 * 1024,
        1 * 1024 * 1024 * 1024LLU,          // 1 GiB
        2 * 1024 * 1024 * 1024LLU,
        4 * 1024 * 1024 * 1024LLU,
        8 * 1024 * 1024 * 1024LLU,
        16 * 1024 * 1024 * 1024LLU,
        32 * 1024 * 1024 * 1024LLU,
        /*64 * 1024 * 1024 * 1024LLU,
        128 * 1024 * 1024 * 1024LLU,
        256 * 1024 * 1024 * 1024LLU,
        512 * 1024 * 1024 * 1024LLU,
        1024 * 1024 * 1024 * 1024LLU,*/       // 1 TiB
        0   // list termination
};

// -----------------------------------------------------------------------------
// --- Main Program

// flag for terminating current test
bool g_done;

// global current number of threads
int g_nthreads = 0;

// synchronization barrier for current thread counter
Barrier *g_barrier;

// thread shared parameters for test function
uint64_t g_thrsize;
uint64_t g_thrsize_spaced;
uint64_t g_repeats;

// Create a one-cycle permutation of pointers in the memory area
void make_cyclic_permutation(int thread_num, void *memarea, size_t bytesize) {
    void **ptrarray = (void **) memarea;
    size_t size = bytesize / sizeof(void *);


    // *** Barrier ****
    g_barrier->wait();


    for (size_t i = 0; i < size; ++i) {
        ptrarray[i] = &ptrarray[i]; // fill area with pointers to self-address
    }


    LCGRandom srnd((size_t) ptrarray + 233349568);

    for (size_t n = size; n > 1; --n) {
        size_t i = srnd() % (n - 1);      // permute pointers to one-cycle
        std::swap(ptrarray[i], ptrarray[n - 1]);
    }

    if (gopt_testcycle) {

        void *ptr = ptrarray[0];
        size_t steps = 1;

        while (ptr != &ptrarray[0] && steps < size * 2) {
            ptr = *(void **) ptr;         // walk pointer
            ++steps;
        }

        assert(steps == size);
    } else {
    }

    // *** Barrier ****
    g_barrier->wait();;

}

void *thread_master(void *cookie) {
    // this weirdness is because (void*) cannot be cast to int and back.
    int thread_num = *((int *) cookie);
    delete (int *) cookie;

    // initial repeat factor is just an approximate B/s bandwidth
    uint64_t factor = 1024 * 1024 * 1024;

    for (const uint64_t *areasize = areasize_list; *areasize; ++areasize) {
        if (*areasize < gopt_sizelimit_min && gopt_sizelimit_min != 0) {
            ocall_print_string(
                    (std::string("Skipping ") + g_func->name + std::string(" test with ") + std::to_string(*areasize) +
                     std::string(" minimum array size due to -s ") + std::to_string(gopt_sizelimit_min) +
                     std::string(".")).c_str());
            continue;
        }
        if (*areasize > gopt_sizelimit_max && gopt_sizelimit_max != 0) {
            ocall_print_string(
                    (std::string("Skipping ") + g_func->name + std::string(" test with ") + std::to_string(*areasize) +
                     std::string(" minimum array size due to -S ") + std::to_string(gopt_sizelimit_max) +
                     std::string(".")).c_str());
            continue;
        }

        for (unsigned int round = 0; round < 1; ++round) {
            // divide area by thread number
            g_thrsize = *areasize / g_nthreads;

            // unrolled tests do up to 16 accesses without loop check, thus align
            // upward to next multiple of unroll_factor*size (e.g. 128 bytes for
            // 16-times unrolled 64-bit access)
            uint64_t unrollsize = g_func->unroll_factor * g_func->bytes_per_access;
            g_thrsize = ((g_thrsize + unrollsize - 1) / unrollsize) * unrollsize;

            // total size tested
            uint64_t testsize = g_thrsize * g_nthreads;

            // skip if tests don't fit into memory
            if (g_memsize < testsize) continue;

            // due to cache thrashing in adjacent cache lines, space out
            // threads's test areas
            g_thrsize_spaced = std::max<uint64_t>(g_thrsize, 4 * 1024 * 1024 + 16 * 1024);

            // skip if tests don't fit into memory
            if (g_memsize < g_thrsize_spaced * g_nthreads) continue;

            g_repeats = (factor + g_thrsize - 1) / g_thrsize;         // round up

            // volume in bytes tested
            uint64_t testvol = testsize * g_repeats * g_func->bytes_per_access / g_func->access_offset;
            // number of accesses in test
            uint64_t testaccess = testsize * g_repeats / g_func->access_offset;
            ocall_print_string(((std::string) "Running "
                                + "nthreads=" + std::to_string(g_nthreads)
                                + " factor=" + std::to_string(factor)
                                + " areasize=" + std::to_string(*areasize)
                                + " thrsize=" + std::to_string(g_thrsize)
                                + " testsize=" + std::to_string(testsize)
                                + " repeats=" + std::to_string(g_repeats)
                                + " testvol=" + std::to_string(testvol)
                                + " testaccess=" + std::to_string(testaccess)).c_str());

            g_done = false;
            double runtime;

            // synchronize with worker threads and run a worker ourselves
            {
                // *** Barrier ****
                g_barrier->wait();

                assert(!g_done);

                // create cyclic permutation for each thread
                if (g_func->make_permutation)
                    make_cyclic_permutation(thread_num, g_memarea + thread_num * g_thrsize_spaced, g_thrsize);

                // *** Barrier ****
                g_barrier->wait();
                uint64_t cpu_counter = 0;
                {
                    rdtscpWrapper rdtscp(&cpu_counter);
                    g_func->func(g_memarea + thread_num * g_thrsize_spaced, g_thrsize, g_repeats);
                }

                // *** Barrier ****
                g_barrier->wait();
                double time_s = (((double) cpu_counter / 2900.0) / 1000000.0);

                runtime = time_s;
            }

            if (runtime < g_min_time) {
                // test ran for less than one second, repeat test and scale
                // repeat factor
                factor = g_thrsize * g_repeats * g_avg_time / runtime;
                ocall_print_string(("run time = " + std::to_string(runtime) + " -> rerunning test with repeat factor=" +
                                    std::to_string(factor)).c_str());

                --round;     // redo this areasize
            } else {
                // adapt repeat factor to observed memory bandwidth, so that
                // next test will take approximately g_avg_time sec

                factor = g_thrsize * g_repeats * g_avg_time / runtime;
                ocall_print_string(("run time = " + std::to_string(runtime) + " -> next test with repeat factor=" +
                                    std::to_string(factor)).c_str());


                result_struct run_result;
                run_result.nthreads = g_nthreads;
                run_result.areasize = *areasize;
                run_result.threadsize = g_thrsize;
                run_result.testsize = testsize;
                run_result.repeats = g_repeats;
                run_result.testvol = testvol;
                run_result.testaccess = testaccess;
                run_result.runtime = runtime;
                run_result.bandwidth = testvol / runtime;
                run_result.rate = runtime / testaccess;
                ocall_print_result(g_func->name,run_result);
            }
        }
    }

    g_done = true;

    // *** Barrier ****
    g_barrier->wait();


    return NULL;
}

void *thread_worker(void *cookie) {
    // this weirdness is because (void*) cannot be cast to int and back.
    int thread_num = *((int *) cookie);
    delete (int *) cookie;

    while (1) {
        // *** Barrier ****
        g_barrier->wait();


        if (g_done) break;

        // create cyclic permutation for each thread
        if (g_func->make_permutation)
            make_cyclic_permutation(thread_num, g_memarea + thread_num * g_thrsize_spaced, g_thrsize);

        // *** Barrier ****
        g_barrier->wait();


        g_func->func(g_memarea + thread_num * g_thrsize_spaced, g_thrsize, g_repeats);

        // *** Barrier ****
        g_barrier->wait();

    }

    return NULL;
}



void testfunc(const TestFunction *func) {
    if (!match_funcfilter(func->name)) {
        ocall_print_string(("Skipping " + std::string(func->name) + " tests").c_str());
        return;
    }

    int nthreads = 1;

    if (gopt_nthreads_min != 0)
        nthreads = gopt_nthreads_min;

    if (gopt_nthreads_max == 0)
        gopt_nthreads_max = g_physical_cpus + 2;

    bool exp_have_physical = false;

    while (1) {
        // globally set test function and thread number
        g_func = func;
        g_nthreads = nthreads;

        // create barrier and run threads
        g_barrier = new Barrier(nthreads);
        pthread_t thr[nthreads];
        int rc = pthread_create(&thr[0], NULL, thread_master, new int(0));
        if (rc) {
            ocall_print_string("Error:unable to create thread");
        }
        for (int p = 1; p < nthreads; ++p)
            pthread_create(&thr[p], NULL, thread_worker, new int(p));

        for (int p = 0; p < nthreads; ++p)
            pthread_join(thr[p], NULL);

        delete g_barrier;
        // increase thread count
        if (nthreads >= gopt_nthreads_max) break;

        if (gopt_nthreads_exponential)
            nthreads = 2 * nthreads;
        else
            nthreads++;

        // Prevent the next check from running the tests with g_physical_cpus
        // twice if that is a power of two
        if (gopt_nthreads_exponential && nthreads == g_physical_cpus)
            exp_have_physical = true;

        if (gopt_nthreads_exponential && nthreads > g_physical_cpus &&
            !exp_have_physical) {
            // Halve it because we want both with and without hyperthreading.
            // The next iteration will then have nthreads == g_physical_cpus,
            // and the one after that will be the last with gopt_nthreads_max.
            nthreads = g_physical_cpus / 2;
            exp_have_physical = true;
        }

        if (nthreads > gopt_nthreads_max)
            nthreads = gopt_nthreads_max;
    }
}

void ecall_main(gopt_parameters params) {
    gopt_sizelimit_min = params.gopt_sizelimit_min;
    gopt_sizelimit_max = params.gopt_sizelimit_max;
    gopt_funcfilter.assign(params.gopt_funcfilter, params.gopt_funcfilter + params.gopt_funcfilter_size);
    gopt_nthreads_max = params.gopt_nthreads_max;
    gopt_nthreads_min = params.gopt_nthreads_min;
    gopt_nthreads_exponential = params.gopt_nthreads_exponential;
    gopt_testcycle = params.gopt_testcycle;
    g_physical_cpus = params.g_physical_cpus;
    g_memsize = params.g_memsize;
    ocall_print_string(("Allocating " + std::to_string(g_memsize / 1024 / 1024) + " MiB for testing.").c_str());
    cpuid_detect();
    // allocate memory area

#if HAVE_POSIX_MEMALIGN

    if (posix_memalign((void**)&g_memarea, 32, g_memsize) != 0) {
        ocall_print_string("Error allocating memory.");
        return -1;
    }

#else

    g_memarea = (char *) aligned_alloc(64, g_memsize);

#endif

    // fill memory with junk, but this allocates physical memory
    memset(g_memarea, 1, g_memsize);

    for (size_t i = 0; i < g_testlist.size(); ++i) {
        TestFunction *tf = g_testlist[i];

        if (!tf->is_supported()) {
            ocall_print_string(("Skipping " + std::string(tf->name) + " test "
                            + "due to missing CPU feature '" + tf->cpufeat + "'.").c_str());
            continue;
        }

        testfunc(tf);
    }

    // cleanup

    free(g_memarea);

    for (size_t i = 0; i < g_testlist.size(); ++i)
        delete g_testlist[i];

};