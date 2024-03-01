# Intel SGXv2 Analytical Query Processing Benchmarks

This repository contains the code used in the VLDB 2024 paper "Benchmarking Analytical Query Processing in Intel SGXv2".

The code is structured into two directories. [Join-Benchmarks](Join-Benchmarks/README.md) contains our join benchmarks. The column scan benchmark and all other micro-benchmarks can be found in [Scan-Micro-Benchmarks](Scan-Micro-Benchmarks/README.md). Both directories contain a more in-depth readme on how to compile and run the code.

This repository contains the exact code and gathered data that was used for the experimental evaluation in our paper. However, it is not yet polished for easy reproducability. Manual steps like copying files to correct directories might be required and the readme files for the benchmarks might be incomplete. We will improve the status over the next two weeks.
