# SGXv2 Benchmarks

This directory contains the column scan, radix partitioning, write and pmbw benchmarks used in the paper. They can be
found inside the [microbenchmarks](microbenchmarks) directory.

## Prerequisites
- Intel SGX SDK for Linux (tested with version 2.21)
- Intel SGX PSW for Linux (tested with version 2.21)
- Linux OS (tested with Ubuntu 22.04.3 LTS, Kernel 6.5)
- Dependencies: cmake, GCC and G++ 12, Python 3, pip, Git
- Python packages: pandas matplotlib seaborn

## Build
To build the code run the following commands in the top-level directory:
1) Configure the build files and put them in folder `cmake-build-release`:
    ```shell
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 -DSGX_MODE=PreRelease -S . -B cmake-build-release
    ```
    To show a list of available targets use the following command:
    ```shell
    cmake --build cmake-build-release --target help
    ```
2) Build all micro-benchmarks using the configuration files in the folder `cmake-build-release`
    ```shell
    cmake --build cmake-build-release --parallel --target all
    ```

## Run

Each subproject inside the [microbenchmarks](microbenchmarks) folder is compiled into its own binary available under
`cmake-build-release/microbenchmarks/[benchmark_name]` after build. They all use gflags and a `--help` flag is provided.
Flags used for benchmarks in the paper can be found in the individual micro-benchmark directories as `.sh` files under 
`results` or `benchmarks`. All binaries return measurements via console output.

For ease of use, we created shell scripts that run an experiment 10 times and store the output in a CSV file. Copy them
from the microbenchmark directory to the directory containing the binary to use them.

In order to plot the results. Copy the result CSV files to the corresponding `results/data` directory, remove double 
header lines and run the python file in the `results` directory.
