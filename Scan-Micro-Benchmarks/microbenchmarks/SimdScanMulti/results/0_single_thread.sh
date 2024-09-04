#!/usr/bin/env bash

echo "Single Thread"
export SGX_DBG_OPTIN=1

if [ -z "$1" ]
then
  result_file="./single-thread.csv"
else
  result_file="$1"
fi

for i in {0..9}
do
  echo "Run $i"
  ./simdmulti --enclave=b --preload=b --mode=bitvector --min_selectivity=1 --max_selectivity=1 --num_reruns=1 \
    --unique_data=f --num_warmup_runs=10 --num_runs=1000 --min_threads=1 --max_threads=1 --min_entries_exp=10 \
    --max_entries_exp=30 | tail -n +$((i > 0 ? 2 : 1)) | tee -a "$result_file"
  ./simdmulti --enclave=b --preload=b --mode=bitvector --min_selectivity=1 --max_selectivity=1 --num_reruns=1 \
    --unique_data=f --num_warmup_runs=10 --num_runs=100 --min_threads=1 --max_threads=1 --min_entries_exp=28 \
    --max_entries_exp=34 | tail -n +2 | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"