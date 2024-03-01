#!/usr/bin/env bash

for i in {6..10}
do
  numactl --physcpubind="$i" --membind=0 -- ./pmbw -M 58G -P 1 -S 0 -f PermRead64SimpleLoop -o pmbw_native_pointer_chasing_"$i".txt
  numactl --physcpubind="$i" --membind=0 -- ./pmbw -M 58G -P 1 -S 0 -f PermRead64SimpleLoop -e -o pmbw_enclave_pointer_chasing_"$i".txt
done
