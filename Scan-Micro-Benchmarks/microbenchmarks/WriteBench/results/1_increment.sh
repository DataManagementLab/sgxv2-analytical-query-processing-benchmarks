#!/usr/bin/env bash

echo "Increment Bench"
export SGX_DBG_OPTIN=1

result_file="./increment.csv"

for i in {0..9}
do
  echo "Run $i"
  numactl --physcpubind="$i" --membind=0 -- ./writebench --mode="inc" --repeat=1 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"
