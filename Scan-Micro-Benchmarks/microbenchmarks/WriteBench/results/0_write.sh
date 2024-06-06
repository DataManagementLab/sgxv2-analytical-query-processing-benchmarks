#!/usr/bin/env bash

echo "Write Bench Area Increase"
export SGX_DBG_OPTIN=1

result_file="./write_area_increase.csv"

for i in {0..9}
do
  echo "Run $i"
  numactl --physcpubind="$i" --membind=0 -- ./writebench --mode="write" --repeat=1 --size_exp_min=3 --size_exp_max=35 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"
