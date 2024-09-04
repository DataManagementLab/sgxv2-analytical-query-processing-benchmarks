# SGXv2 Benchmarks

This directory contains the column scan, histogram, and read/write benchmarks used in the paper. They can be
found inside the [microbenchmarks](microbenchmarks) directory.

## Prerequisites
- Intel SGX SDK for Linux (tested with version 2.24)
- Intel SGX PSW for Linux (tested with version 2.24)
- Linux OS (tested with Ubuntu 22.04.3 LTS, Kernel 6.5)
- Dependencies: cmake, GCC and G++ 12, Python 3, pip, Git
- Python package as defined in `requirements.txt`

## Build
To build the code, run the following commands in the top-level directory:
1) Configure the build files and put them in folder `cmake-build-release`:
    ```shell
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 -DSGX_MODE=PreRelease -S . -B cmake-build-release
    ```
    To show a list of available targets use the following command:
    ```shell
    cmake --build cmake-build-release --target help
    ```
2) Build all benchmarks using the configuration files in the folder `cmake-build-release`
    ```shell
    cmake --build cmake-build-release --parallel --target all
    ```

## Run

Each subproject inside the [microbenchmarks](microbenchmarks) folder is compiled into its own binary available under
`cmake-build-release/microbenchmarks/[benchmark_name]` after build. They all use gflags and a `--help` flag is provided.
For ease of use, we created shell scripts that run an experiment 10 times and store the output in a CSV file. They are 
automatically copied to the binary directory and are named with an integer followed by a short experiment name. For
example, `cmake-build-release/microbenchmarks/SimdScanMulti` contains the experiment scripts `0_single_thread.sh` and
`1_scale_up.sh`. The experiment scripts expect that the working directory is their local directory.

In order to plot the results. Copy the result CSV files to the corresponding `results/data` directory and run 
`results/plot.py`. Like the experiment scripts, the python scripts expect that they are executed in their directory.

### Example

To reproduce the column scan benchmarks, first switch to the binary directory after building:

```shell
cd cmake-build-release/microbenchmarks/SimdScanMulti
```

Then run the experiment scripts:

```shell
./0_single_thread.sh &&\
./1_scale_up.sh &&\
./2_write_rate.sh &&\
./3_numa.sh
```

Then move/copy the result files to the source directory `microbenchmarks/SimdScanMulti/results/data`:

```shell
mv single-thread.csv scale-up.csv write-rate.csv cross-numa.csv ../../../microbenchmarks/SimdScanMulti/results/data
```

Finally, plot the results:

```shell
cd ../../../microbenchmarks/SimdScanMulti/results
python3 plot.py
```

The result figures are stored in `img`. The other results from `RadixPartitioning` and `WriteBench` can be 
reproduced with the same process.

### Exception

To reproduce the SIMD scan experiments with 16-bit input values, checkout the branch `16-bit` and run 
`0_single_thread.sh single-thread-16bit.csv`.
