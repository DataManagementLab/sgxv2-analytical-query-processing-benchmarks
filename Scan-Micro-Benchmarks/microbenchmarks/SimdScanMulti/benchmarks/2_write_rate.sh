#!/usr/bin/env bash

echo "Write Rate"
for i in {0..9}
do
  echo "$i"
  ./simdmulti --enclave=b --preload=b --mode=noIndex --min_selectivity=1 --max_selectivity=1 --unique_data=t \
  --min_threads=16 --max_threads=16 --min_entries_exp=32 --max_entries_exp=32 | tee -a ./write-rate.csv
  ./simdmulti --enclave=b --preload=b --mode=noIndex --min_selectivity=10 --max_selectivity=100 --step_selectivity=10 \
  --unique_data=t --min_threads=16 --max_threads=16 --min_entries_exp=32 --max_entries_exp=32 | tee -a ./write-rate.csv
done
