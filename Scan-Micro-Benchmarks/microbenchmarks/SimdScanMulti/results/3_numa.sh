#!/usr/bin/env bash

echo "NUMA"
export SGX_DBG_OPTIN=1

result_file="./cross-numa.csv"

for i in {0..9}
do
  echo "Run $i"
  ./simdmulti --enclave=b --preload=t --mode=bitvector --num_reruns=1 --min_selectivity=1 --max_selectivity=1 \
  --unique_data=t --min_threads=1 --max_threads=16 --min_entries_exp=34 --max_entries_exp=34 --numa=b \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"