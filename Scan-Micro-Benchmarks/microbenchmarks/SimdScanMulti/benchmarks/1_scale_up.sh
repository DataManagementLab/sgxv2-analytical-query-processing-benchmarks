#!/usr/bin/env bash

echo "Scale Up"
for i in {0..9}
do
  echo "$i"
  ./simdmulti --enclave=b --preload=b --mode=bitvector --min_selectivity=1 --max_selectivity=1 --unique_data=t \
  --min_threads=1 --max_threads=16 --min_entries_exp=34 --max_entries_exp=34 | tee -a ./scale-up.csv
done
