# Intel SGXv2 Analytical Query Processing Benchmarks

This repository contains the code used in the EDBT 2025 paper ["Benchmarking Analytical Query Processing in Intel SGXv2"](https://openproceedings.org/2025/conf/edbt/paper-71.pdf).

## Abstract

Trusted Execution Environments (TEEs), such as Intel’s Software Guard Extensions (SGX), are increasingly being adopted to address trust and compliance issues in the public cloud. Intel SGX’s second generation (SGXv2) addresses many limitations of its predecessor (SGXv1), offering the potential for secure and efficient analytical cloud DBMSs. We assess this potential and conduct the first in-depth evaluation study of analytical query processing algorithms inside SGXv2. Our study reveals that, unlike SGXv1, state-of-the-art algorithms like radix joins and SIMD-based scans are a good starting point for achieving high-performance query processing inside SGXv2. However, subtle hardware and software differences still influence code execution inside SGX enclaves and cause substantial overheads. We investigate these differences and propose new optimizations to bring the performance inside enclaves on par with native code execution outside enclaves.

## Citation

```text
@inproceedings{DBLP:conf/edbt/LutschEH0IB25,
  author       = {Adrian Lutsch and
                  Muhammad El{-}Hindi and
                  Matthias Heinrich and
                  Daniel Ritter and
                  Zsolt Istv{\'{a}}n and
                  Carsten Binnig},
  editor       = {Alkis Simitsis and
                  Bettina Kemme and
                  Anna Queralt and
                  Oscar Romero and
                  Petar Jovanovic},
  title        = {Benchmarking Analytical Query Processing in Intel SGXv2},
  booktitle    = {Proceedings 28th International Conference on Extending Database Technology,
                  {EDBT} 2025, Barcelona, Spain, March 25-28, 2025},
  pages        = {516--528},
  publisher    = {OpenProceedings.org},
  year         = {2025},
  url          = {https://doi.org/10.48786/edbt.2025.41},
  doi          = {10.48786/EDBT.2025.41},
  timestamp    = {Wed, 20 Nov 2024 11:42:44 +0100},
  biburl       = {https://dblp.org/rec/conf/edbt/LutschEH0IB25.bib},
  bibsource    = {dblp computer science bibliography, https://dblp.org}
}
```

## Repository Overview

The code is structured into two directories. [Join-Benchmarks](Join-Benchmarks/README.md) contains our join benchmarks. The column scan benchmark and all other micro-benchmarks can be found in [Scan-Micro-Benchmarks](Scan-Micro-Benchmarks/README.md). Both directories contain a more in-depth readme on how to compile and run the code.

## Run Experiments

Please refer to the details in the sub-directories [Join-Benchmarks](Join-Benchmarks/README.md) and [Scan-Micro-Benchmarks](Scan-Micro-Benchmarks/README.md)

