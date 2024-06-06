#!/usr/bin/env bash

echo "Plotting all join figures"

python3 paper-0-intro.py plot intro
python3 paper-1-join-algorithm-overview.py plot join-overview
python3 paper-2-random-memory-access-pht.py plot random-access-pht
python3 paper-3-RHO-phases.py plot RHO-phases
python3 paper-4-rho-numa.py plot rho-numa
python3 paper-6-mutex-contention.py plot mutex-contention
python3 paper-7-slow-malloc.py plot slow-malloc
python3 paper-8-full-query-optimization-impact.py plot full-query-optimization-impact
python3 paper-9-unroll-reorder-improvements.py plot unroll-reorder-improvements-mitigation

echo "Done."