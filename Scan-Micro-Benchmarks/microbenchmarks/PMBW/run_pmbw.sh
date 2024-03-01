#!/usr/bin/env bash

for i in {1..10}
do
  numactl --physcpubind=0-15 --membind=0 -- ./pmbw -M 58G -p 16 -P 16 -S 0 -f ScanRead512PtrSimpleLoop -f ScanRead64PtrSimpleLoop -f ScanWrite512PtrSimpleLoop -f ScanWrite64PtrSimpleLoop -f PermRead64SimpleLoop -f PermRead64UnrollLoop -o pmbw_native_stats_large_"$i".txt
  numactl --physcpubind=0-15 --membind=0 -- ./pmbw -M 58G -p 16 -P 16 -S 0 -f ScanRead512PtrSimpleLoop -f ScanRead64PtrSimpleLoop -f ScanWrite512PtrSimpleLoop -f ScanWrite64PtrSimpleLoop -f PermRead64SimpleLoop -f PermRead64UnrollLoop -e -o pmbw_enclave_stats_large_"$i".txt
done
