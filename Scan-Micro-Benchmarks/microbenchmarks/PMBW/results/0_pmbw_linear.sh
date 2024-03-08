#!/usr/bin/env bash

echo "PMBW Linear Read/Write Benchmarks"

for i in {1..10}
do
  numactl --physcpubind=0-15 --membind=0 -- ./pmbw -M 58G -p 16 -P 16 -S 0 \
  -f ScanRead512PtrSimpleLoop -f ScanRead64PtrSimpleLoop -f ScanWrite512PtrSimpleLoop -f ScanWrite64PtrSimpleLoop \
  -o pmbw_linear_native_"$i".txt
  numactl --physcpubind=0-15 --membind=0 -- ./pmbw -M 58G -p 16 -P 16 -S 0 \
  -f ScanRead512PtrSimpleLoop -f ScanRead64PtrSimpleLoop -f ScanWrite512PtrSimpleLoop -f ScanWrite64PtrSimpleLoop \
  -e -o pmbw_liner_enclave_"$i".txt
done

echo "Done. Stored results in pmbw_linear_(native|enclave)_[1-10].txt. Use pmbw_convert_csv.py to convert to csv."