#!/usr/bin/env bash

echo "Write Rate"
export SGX_DBG_OPTIN=1

result_file="./write-rate.csv"

for i in {0..9}
do
  echo "Run $i"
  ./simdmulti --enclave=b --preload=b --mode=noIndex --min_selectivity=1 --max_selectivity=1 --unique_data=t \
  --min_threads=16 --max_threads=16 --min_entries_exp=32 --max_entries_exp=32 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
  ./simdmulti --enclave=b --preload=b --mode=noIndex --min_selectivity=10 --max_selectivity=100 --step_selectivity=10 \
  --unique_data=t --min_threads=16 --max_threads=16 --min_entries_exp=32 --max_entries_exp=32 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"