#!/usr/bin/env bash

echo "Running all benchmarks"

numactl --physcpubind=0-15 --membind=0 python3 paper-0-intro.py run intro
numactl --physcpubind=0-15 --membind=0 python3 paper-1-join-algorithm-overview.py run join-overview
numactl --physcpubind=0-15 --membind=0 python3 paper-2-random-memory-access-pht.py run random-access-pht
numactl --physcpubind=0-15 --membind=0 python3 paper-3-RHO-phases.py run RHO-phases
numactl --physcpubind=0-15 --membind=0 python3 paper-4-scaling.py run scaling-perf
numactl --physcpubind=0-31 python3 paper-5-rho-numa.py run rho-numa
numactl --physcpubind=0-15 --membind=0 python3 paper-6-mutex-contention.py run mutex-contention
numactl --physcpubind=0-15 --membind=0 python3 paper-7-slow-malloc.py run slow-malloc
numactl --physcpubind=0-15 --membind=0 python3 paper-8-full-query-optimization-impact.py run full-query-optimization-impact
numactl --physcpubind=0-15 --membind=0 python3 paper-revision-9-skew.py run skew

echo "Done."
