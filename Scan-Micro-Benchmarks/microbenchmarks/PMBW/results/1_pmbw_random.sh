#!/usr/bin/env bash

echo "PMBW Random Read Benchmarks"

for i in {1..10}
do
  numactl --physcpubind="$i" --membind=0 -- ./pmbw -M 58G -P 1 -S 0 -f PermRead64SimpleLoop -o pmbw_random_native_"$i".txt
  numactl --physcpubind="$i" --membind=0 -- ./pmbw -M 58G -P 1 -S 0 -f PermRead64SimpleLoop -e -o pmbw_random_enclave_"$i".txt
done

echo "Done. Stored results in pmbw_random_(native|enclave)_[1-10].txt. Use pmbw_convert_csv.py to convert to csv."
