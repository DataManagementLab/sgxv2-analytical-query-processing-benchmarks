#ifndef SGX_ENCRYPTEDDB_STRUCTS_H
#define SGX_ENCRYPTEDDB_STRUCTS_H
// -----------------------------------------------------------------------------
// --- Registry for Memory Testing Functions

typedef void (*testfunc_type)(char* memarea, size_t size, size_t repeats);


struct TestFunction {
    // identifier of the test function
    char *name;

    // function to call
    testfunc_type func;

    // prerequisite CPU feature
    const char *cpufeat;

    // number of bytes read/written per access (for latency calculation)
    unsigned int bytes_per_access;

    // bytes skipped foward to next access point (including bytes_per_access)
    unsigned int access_offset;

    // number of accesses before and after
    unsigned int unroll_factor;

    // fill the area with a permutation before calling the func
    bool make_permutation;

    // constructor which also registers the function
    TestFunction(char *n, testfunc_type f, const char *cf,
                 unsigned int bpa, unsigned int ao, unsigned int unr,
                 bool mp);

    // test CPU feature support
    bool is_supported() const;
};
std::vector<TestFunction*> g_testlist;

TestFunction::TestFunction(char *n, testfunc_type f, const char *cf,
                           unsigned int bpa, unsigned int ao, unsigned int unr,
                           bool mp)
        : name(n), func(f), cpufeat(cf),
          bytes_per_access(bpa), access_offset(ao), unroll_factor(unr),
          make_permutation(mp) {
    g_testlist.push_back(this);
}

/*
struct Result {
    const char *funcname;
    int nthreads;
    uint64_t areasize;
    uint64_t threadsize;
    uint64_t testsize;
    uint64_t repeats;
    uint64_t testvol;
    uint64_t testaccess;
    double runtime;
    double bandwidth;
    double rate;
};*/
/*
struct gopt_parameters{
// filter of functions to run, set by command line
    char** gopt_funcfilter = {};
    size_t gopt_funcfilter_size = 0;
// set default size limit: 0 -- 4 GiB
    uint64_t gopt_sizelimit_min = 0;
    uint64_t gopt_sizelimit_max = 4*1024*1024*1024LLU;

// set memory limit
    uint64_t gopt_memlimit = 0;

// lower and upper limit to number of threads
    int gopt_nthreads_min = 0, gopt_nthreads_max = 0;

// exponentially increasing number of threads
    int gopt_nthreads_exponential = false;

// option to test permutation cycle before measurement
    int gopt_testcycle = false;

// option to change the output file from default "stats.txt"
    const char* gopt_output_file = "stats.txt";
};*/
#endif //SGX_ENCRYPTEDDB_STRUCTS_H
