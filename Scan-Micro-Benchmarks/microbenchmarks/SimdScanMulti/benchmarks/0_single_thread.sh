#!/usr/bin/env bash

echo "Single Thread"
for i in {0..9}
do
  echo "$i"
  ./simdmulti --enclave=b --preload=b --mode=bitvector --min_selectivity=1 --max_selectivity=1 --num_reruns=1 \
  --unique_data=f --num_warmup_runs=10 --num_runs=1000 --min_threads=1 --max_threads=1 --min_entries_exp=10 \
  --max_entries_exp=34 | tee -a ./single-thread.csv
done
