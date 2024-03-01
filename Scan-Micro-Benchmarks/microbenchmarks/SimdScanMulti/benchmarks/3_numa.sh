#!/usr/bin/env bash

echo "NUMA"
for i in {0..9}
do
  echo "$i"
  ./simdmulti --enclave=b --preload=b --mode=bitvector --num_reruns=1 --min_selectivity=1 --max_selectivity=1 \
  --unique_data=t --min_threads=1 --max_threads=16 --min_entries_exp=34 --max_entries_exp=34 --numa=b \
  | tee -a ./cross-numa.csv
done
