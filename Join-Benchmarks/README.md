# SGXv2 Join Benchmarks derived from TeeBench

This directory contains a stripped-down and enhanced version of [TEEBench](https://github.com/agora-ecosystem/tee-bench)
by Maliszewski et al. We used this code for benchmarking joins in SGXv2.

## Main Changes from Original TEEBench

- Removed memory access oblivious joins, FHE joins, sealing joins, 3-way joins, grace, MC, and stitch join
- Removed single-threaded radix join
- Removed PCM, functionality based on custom SGX driver, and functionality based on custom SGX SDK
- Removed paper PDF
- Removed TEEBench experiment Scripts
- Changed the build system to CMake and restructured the whole repository into libraries
- Integrated [CrkJoin](https://github.com/kai-chi/CrkJoin) into this version of TEEBench
- Added a TPC-H library and with TPC-H queries using SIMD scans and the RHO join
- Replaced the mechanism for result materialization in TEEBench (one linked list per thread, one malloc per row) with a 
  more performant mechanism (table consisting of 16 kB chunks).
- Removed RHO_atomic
- Radix joins now use boost::lockfree::queue by default
- Made number of radix bits in radix joins dynamic
- Implemented unrolling and instruction reordering in radix and hash joins
- Added compile time options controlling the partitioning behavior of radix joins
- Replaced pthreads barrier implementation with a barrier using busy waiting on atomics
- Added more fine-grained time measurements for different join phases
- Added time measurements via rdtscp instead of OCALLs
- Added an option to activate mitigation for the SSB attack outside an enclave.
- Added PerfEvent as hardware performance counter measurement tool
- Added -march=native to compile options
- Added [x86 simd sort](https://github.com/intel/x86-simd-sort) to RSM
- Added SGXv2 experiment scripts

## Prerequisites

- Intel SGX SDK for Linux (tested with version 2.24)
- Intel SGX PSW for Linux (tested with version 2.24)
- Linux OS (tested with Ubuntu 22.04.4 LTS, Kernel 6.5)
- Dependencies: cmake, make or ninja, GCC and G++ 12, Python 3, pip, Git
- Boost 1.84 installed in `/opt/boost_1_84_0`. [Download](https://www.boost.org/users/download/)
- Python packages as defined in `requirements.txt`
- For full query experiments: TPC-H `.tbl` files in `data/scale###` where `###` is the scale factor with leading zeros.
  TPC-H dbgen is available [here](https://www.tpc.org/tpc_documents_current_versions/current_specifications5.asp).

## Preparation (only TPC-H experiments)

For the full query experiments (`paper-8-full-query-optimization-impact`), the TPC-H tables must be converted to binary
format:

1. First make sure you have the required TPC-H `.tbl` files for scale factors 1, 10 and 100 in `data/scale###` where
   `###` is the scale factor with leading zeros.
2. Then compile the CSV to binary converter:
```shell
cmake -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 -DCMAKE_BUILD_TYPE=Release -B cmake-build-release \
&& cmake --build cmake-build-release --target csv_convert
```
3. Then run the converter script next to the binary. This creates the binary tables for scale factor 1, 10 and 100. It
   can take a while.
```shell
cd cmake-build-release && ./create_binary_tables.sh
```

## Run the Experiments

1. Make sure you have all dependencies, especially the Intel SGX SDK, installed and enabled on your machine.
2. Activate a Python environment that fulfills `requirements.txt`.
3. Go to `SGXv2Scripts/scripts`.
4. Run any of the scripts with the `both` parameter. The scripts compile the code with the required flags and run the 
   required settings 10 times. The result csv files are stored in `data` and the figures in `img`. In order to not 
   measure the effects of Hyper-Threading or NUMA, we pinned all experiments to one NUMA node. Example: 
   `numactl --physcpubind=0-15 --membind=0 -- python3 paper-0-intro.py both`. The NUMA experiment was pinned to the 
   first 32 cores: `numactl --physcpubind=0-31 -- python3 paper-4-rho-numa.py both`.

The steps below are not required for reproducing the paper results, as they are done automatically by the experiment
scripts. They are meant for documentation.

## Compilation

The project uses CMAKE as build tool. There are 4 main targets:

1. `teebench` compiles TEEBench for benchmarking joins inside the SGX enclave
2. `native` compiles TEEBench for benchmarking joins outside the enclave
3. `tpch` compiles the TPC-H queries for benchmarking them inside the SGX enclave
4. `tpch-native` compiles the TPC-H queries for benchmarking them outside the enclave

To configure the build, use the following command, where FLAGS is a list of compile flags (see below) separated by 
semicolons and CPMS is the number of CPU cycles per microsecond of your CPU (2900 by default). ENCLAVE_CONFIG_FILE can
be replaced with another configuration if more memory is required.

```shell
cmake -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 -DCMAKE_BUILD_TYPE=Release -DCFLAGS="FLAGS" -DCPMS=CPMS \
-DENCLAVE_CONFIG_FILE=Enclave/Enclave.config.xml -B cmake-build-release
```

To build all targets, use
```shell
cmake --build cmake-build-release
```

To build one of the targets, use
```shell
cmake --build cmake-build-release --target TARGET
```
This creates a binary named cmake-build-release/TARGET

### Important Compile Flags

These flags can be set during configuration via the CMAKE option CFLAGS:

* `UNROLL` - activates unrolling optimization in radix joins
* `MUTEX_QUEUE` - replaces the lock-free queue used in the radix join implementation with the mutex-protected queue from
  TEEBench
* `CONSTANT_RADIX_BITS` - forces usage of 14 radix bits independent of table sizes. Recreates the original behavior of
  TEEBench implementation and forces contention on the task queue.
* `FORCE_2_PHASES` - forces 2-phase radix partitioning, although one phase would suffice used by default in paper
  experiments
* `CHUNKED_TABLE` - replaces the linked list output of the joins with a table consisting of chunks. Default in the paper
* `SIMD` - activates multi-thread SIMD scans in the TPC-H implementations. Default in the paper.

Example: `-DCFLAGS="SIMD;MUTEX_QUEUE"`

## Execution

### Important command line arguments for TEEBench

The currently working list of command line arguments:

* `-a` - join algorithm name. Tested working: `CHT`, `PHT`, `MWAY`, `RHO`, `RHT`, `RSM`, `INL`. Default: `RHO`
* `-n` - number of threads used to execute the join algorithm. Default: `2`
* `-r` - number of tuples of R relation. Default: `2097152`. R is hashed
* `-s` - number of tuples of S relation. Default: `2097152`. S is probed
* `-x` - size of R in MBs. Default: `none`
* `-y` - size of S in MBs. Default: `none`

The currently working list of command line flags:

* `-m` - materialize output tables. Default: `false`
* `--mitigation` activates the SSB mitigation for the experiment. Only has an effect in `native`. Default: `false`

### Important command line arguments for TPC-H

The currently working list of command line arguments:

* `-a` - join algorithm name. Tested working: `CHT`, `PHT`, `MWAY`, `RHO`, `RHT`, `RSM`, `INL`. Default: `RHO`
* `-n` - number of threads used to execute the join algorithm. Default: `2`
* `-q` - Query to execute. One of `3`, `10`, `12`, `19`
* `-s` - Scale factor. Make sure that you have the required tables in `data/scale###`

## Links

- [TEEBench Publication](https://dl.acm.org/doi/10.14778/3494124.3494146)
- [TeeBench Repository](https://github.com/agora-ecosystem/tee-bench)
- [Crkjoin Publication](https://dl.acm.org/doi/10.14778/3598581.3598602)
- [CrkJoin Repository](https://github.com/kai-chi/CrkJoin)
